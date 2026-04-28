#include <immintrin.h>
#include <thread>
#include <atomic>
#include <iostream>
#include <vector>
#include <cstdint>
#include <array>
#include <cstring>
#include "../utils.h"

using v256 = __m256i;

#define ROTR(x,n) ( \
    ((uint32_t)(x) >> (n)) | \
    ((uint32_t)(x) << (32 - (n))) \
)
#define U2(x,y,z) (((x) & (y)) ^ (~(x) & (z)))
#define U1(x,y,z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define V1(x) (ROTR(x,2) ^ ROTR(x,13) ^ ROTR(x,22))
#define V2(x) (ROTR(x,6) ^ ROTR(x,11) ^ ROTR(x,25))
#define S0(x) (ROTR(x,7) ^ ROTR(x,18) ^ (x >> 3))
#define S1(x) (ROTR(x,17) ^ ROTR(x,19) ^ (x >> 10))

#define ADD(a,b)  _mm256_add_epi32((a),(b))
#define XOR(a,b)  _mm256_xor_si256((a),(b))
#define AND(a,b)  _mm256_and_si256((a),(b))
#define OR(a,b)   _mm256_or_si256((a),(b))
#define SHR2(x,n)  _mm256_srli_epi32((x),(n))
#define SHL2(x,n)  _mm256_slli_epi32((x),(n))
#define ROTR2(x,n) OR(SHR2((x),(n)), SHL2((x),(32-(n))))
#define CH(x,y,z)  XOR(AND((x),(y)), AND(_mm256_xor_si256((x),_mm256_set1_epi32(0xFFFFFFFF)),(z)))
#define MAJ(x,y,z) XOR(XOR(AND((x),(y)), AND((x),(z))), AND((y),(z)))
#define BSIG0(x) XOR(XOR(ROTR2((x),2), ROTR2((x),13)), ROTR2((x),22))
#define BSIG1(x) XOR(XOR(ROTR2((x),6), ROTR2((x),11)), ROTR2((x),25))
#define SSIG0(x) XOR(XOR(ROTR2((x),7), ROTR2((x),18)), SHR2((x),3))
#define SSIG1(x) XOR(XOR(ROTR2((x),17), ROTR2((x),19)), SHR2((x),10))

std::atomic<bool> nonce_found{false};
std::atomic<uint32_t> found_nonce{0};
//const int NUM_THREADS = std::thread::hardware_concurrency();
const int NUM_THREADS = 8;
constexpr uint32_t STEP_NONCE = 0x04000000;
constexpr uint32_t START_NONCE = 0x20000000;
constexpr uint32_t TARGET = 0x0001e605;

std::array<uint32_t, 4> statistic;

constexpr std::array<uint32_t, 8> H = {
    0x6A09E667, 0xBB67AE85, 0x3C6EF372, 0xA54FF53A, 0x510E527F, 0x9B05688C, 0x1F83D9AB, 0x5BE0CD19
};

std::array<uint32_t, 8> WH = {
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000
};

constexpr std::array<uint32_t, 64> K = {
    0x428A2F98, 0x71374491, 0xB5C0FBCF, 0xE9B5DBA5, 0x3956C25B, 0x59F111F1, 0x923F82A4, 0xAB1C5ED5,
    0xD807AA98, 0x12835B01, 0x243185BE, 0x550C7DC3, 0x72BE5D74, 0x80DEB1FE, 0x9BDC06A7, 0xC19BF174,
    0xE49B69C1, 0xEFBE4786, 0x0FC19DC6, 0x240CA1CC, 0x2DE92C6F, 0x4A7484AA, 0x5CB0A9DC, 0x76F988DA,
    0x983E5152, 0xA831C66D, 0xB00327C8, 0xBF597FC7, 0xC6E00BF3, 0xD5A79147, 0x06CA6351, 0x14292967,
    0x27B70A85, 0x2E1B2138, 0x4D2C6DFC, 0x53380D13, 0x650A7354, 0x766A0ABB, 0x81C2C92E, 0x92722C85,
    0xA2BFE8A1, 0xA81A664B, 0xC24B8B70, 0xC76C51A3, 0xD192E819, 0xD6990624, 0xF40E3585, 0x106AA070,
    0x19A4C116, 0x1E376C08, 0x2748774C, 0x34B0BCB5, 0x391C0CB3, 0x4ED8AA4A, 0x5B9CCA4F, 0x682E6FF3,
    0x748F82EE, 0x78A5636F, 0x84C87814, 0x8CC70208, 0x90BEFFFA, 0xA4506CEB, 0xBEF9A3F7, 0xC67178F2
};

std::array<uint32_t, 16> w0 = {
    0x00000000, 0x00000000, 0x00000000, 0x00000000, // Version, prevBlock[:96 bit]
    0x80000000, 0x00000000, 0x00000000, 0x00000000, // prevBlock[96 bit:224 bit]
    0x00000000, 0x00000000, 0x00000000, 0x00000000, // prevBlock[224 bit:256 bit], MercleRoot[:96 bit]
    0x00000000, 0x00000000, 0x00000000, 0x00000000  // MercleRoot[96 bit:224 bit]
};

std::array<uint32_t, 16> w1 = {
    0x00000000, 0x00000000, 0x00000000, 0x00000000, // MercleRoot[224 bit:256 bit], Time, Bits, Nonce
    0x80000000, 0x00000000, 0x00000000, 0x00000000, // 1 bit + zero <- const
    0x00000000, 0x00000000, 0x00000000, 0x00000000, // const
    0x00000000, 0x00000000, 0x00000000, 0x00000280  // 0x280 -> 640 bit = 80 byte <- const
};

void set_Version(std::vector<uint8_t>& version) {
    w0[0] = 
        (uint32_t(version[0]) << 24) |
        (uint32_t(version[1]) << 16) |
        (uint32_t(version[2]) <<  8) |
        (uint32_t(version[3]) <<  0);
}

void set_prevBlock(std::vector<uint32_t> prevBlock) {
    for (int i = 0; i < 8; i ++) {
        w0[i+1] = prevBlock[i];
    }
}

void set_MercleRoot(std::vector<uint32_t> mercleRoot) {
    for (int i = 0; i < 7; i ++) {
        w0[i+9] = mercleRoot[i];
    }
    w1[0] = mercleRoot[7];
}

void set_Time(std::vector<uint8_t>& time) {
    w1[1] = 
        (uint32_t(time[0]) << 24) |
        (uint32_t(time[1]) << 16) |
        (uint32_t(time[2]) <<  8) |
        (uint32_t(time[3]) <<  0);
}

void set_Bits(std::vector<uint8_t>& bits) {
    w1[2] = 
        (uint32_t(bits[0]) << 24) |
        (uint32_t(bits[1]) << 16) |
        (uint32_t(bits[2]) <<  8) |
        (uint32_t(bits[3]) <<  0);
}

inline __attribute__((target("avx2"))) v256 make_nonce_vec(uint32_t start_nonce) {
    return _mm256_set_epi32(
        start_nonce + 7*STEP_NONCE, start_nonce + 6*STEP_NONCE, start_nonce + 5*STEP_NONCE, start_nonce + 4*STEP_NONCE,
        start_nonce + 3*STEP_NONCE, start_nonce + 2*STEP_NONCE, start_nonce + STEP_NONCE, start_nonce
    );
}

inline void sha256_scalar_w0() {
    uint32_t a=H[0], b=H[1], c=H[2], d=H[3];
    uint32_t e=H[4], f=H[5], g=H[6], h=H[7];

    for (uint8_t i = 0; i < 64; i++) {
        uint32_t t1 = h + V2(e) + U2(e,f,g) + K[i] + w0[i & 0xF];
        uint32_t t2 = V1(a) + U1(a,b,c);
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
        if (i < 48) {
            w0[i & 0xF] += S1(w0[(i-2) & 0xF]) + w0[(i-7) & 0xF] + S0(w0[(i+1) & 0xF]);
        }
    }

    WH[0] = a + H[0]; WH[1] = b + H[1]; WH[2] = c + H[2]; WH[3] = d + H[3];
    WH[4] = e + H[4]; WH[5] = f + H[5]; WH[6] = g + H[6]; WH[7] = h + H[7];
}

// inline void prev_SHA256() {
//     sha256_scalar_w0();

//     uint32_t temp_c = WH[2], temp_g = WH[6];
//     uint32_t temp_d = WH[3], temp_h = WH[7];
//     WH[2] = WH[0]; WH[3] = WH[1];
//     WH[6] = WH[4]; WH[7] = WH[5];

//     temp_h += K[0] + w1[0] + V2(WH[6]) + U2(WH[6],WH[7],temp_g);
//     WH[1] = temp_h + V1(WH[2]) + U1(WH[2],WH[3],temp_c);
//     WH[5] = temp_d + temp_h;
//     w1[0] += S1(w1[14]) + w1[9] + S0(w1[1]);
//     temp_g += K[1] + w1[1] + V2(WH[5]) + U2(WH[5],WH[6],WH[7]);
//     WH[0] = temp_g + V1(WH[1]) + U1(WH[1],WH[2],WH[3]);
//     WH[4] = temp_c + temp_g;
//     w1[1] += S1(w1[15]) + w1[10] + S0(w1[2]);
// }

inline __attribute__((target("avx2"))) int double_sha256_simd(v256 n) {
    alignas(32) v256 w_simd[16];
    v256 a=_mm256_set1_epi32(WH[0]), b=_mm256_set1_epi32(WH[1]), c=_mm256_set1_epi32(WH[2]), d=_mm256_set1_epi32(WH[3]);
    v256 e=_mm256_set1_epi32(WH[4]), f=_mm256_set1_epi32(WH[5]), g=_mm256_set1_epi32(WH[6]), h=_mm256_set1_epi32(WH[7]);
    w_simd[0] = _mm256_set1_epi32(w1[0]);
    w_simd[1] = _mm256_set1_epi32(w1[1]);
    w_simd[2] = _mm256_set1_epi32(w1[2]);
    w_simd[3] = n;
    for (uint8_t i = 4; i < 16; i ++) {
        w_simd[i] = _mm256_set1_epi32(w1[i]);
    }

    for (uint8_t i = 0; i < 64; i++) {
        v256 t1 = ADD(_mm256_set1_epi32(K[i]), ADD(ADD(w_simd[i & 0xF], h), ADD(BSIG1(e), CH(e,f,g))));
        v256 t2 = ADD(BSIG0(a), MAJ(a,b,c));
        h = g;
        g = f;
        f = e;
        e = ADD(d, t1);
        d = c;
        c = b;
        b = a;
        a = ADD(t1, t2);
        if (i < 48) {
            w_simd[i & 0xF] = ADD(ADD(SSIG1(w_simd[(i-2) & 0xF]), SSIG0(w_simd[(i+1) & 0xF])), ADD(w_simd[i & 0xF], w_simd[(i-7) & 0xF]));
        }
    }

    w_simd[0] = ADD(a, _mm256_set1_epi32(WH[0])); w_simd[1] = ADD(b, _mm256_set1_epi32(WH[1]));
    w_simd[2] = ADD(c, _mm256_set1_epi32(WH[2])); w_simd[3] = ADD(d, _mm256_set1_epi32(WH[3]));
    w_simd[4] = ADD(e, _mm256_set1_epi32(WH[4])); w_simd[5] = ADD(f, _mm256_set1_epi32(WH[5]));
    w_simd[6] = ADD(g, _mm256_set1_epi32(WH[6])); w_simd[7] = ADD(h, _mm256_set1_epi32(WH[7]));
    w_simd[8] = _mm256_set1_epi32(0x80000000); w_simd[15] = _mm256_set1_epi32(0x00000100); // 1 bit + zero <- const <=> 0x100 -> 256 bit = 32 byte <- const
    w_simd[10] = _mm256_setzero_si256(); w_simd[9] = _mm256_setzero_si256();
    w_simd[12] = _mm256_setzero_si256(); w_simd[11] = _mm256_setzero_si256();
    w_simd[14] = _mm256_setzero_si256(); w_simd[13] = _mm256_setzero_si256();

    a = _mm256_set1_epi32(H[0]), b = _mm256_set1_epi32(H[1]), c = _mm256_set1_epi32(H[2]); d = _mm256_set1_epi32(H[3]);
    e = _mm256_set1_epi32(H[4]), f = _mm256_set1_epi32(H[5]), g = _mm256_set1_epi32(H[6]), h = _mm256_set1_epi32(H[7]);

    for (uint8_t i = 0; i < 64; i++) {
        v256 t1 = ADD(_mm256_set1_epi32(K[i]), ADD(ADD(w_simd[i & 0xF], h), ADD(BSIG1(e), CH(e,f,g))));
        v256 t2 = ADD(BSIG0(a), MAJ(a,b,c));
        h = g;
        g = f;
        f = e;
        e = ADD(d, t1);
        d = c;
        c = b;
        b = a;
        a = ADD(t1, t2);
        if (i < 48) {
            w_simd[i & 0xF] = ADD(ADD(SSIG1(w_simd[(i-2) & 0xF]), SSIG0(w_simd[(i+1) & 0xF])), ADD(w_simd[i & 0xF], w_simd[(i-7) & 0xF]));
        }
    }

    w_simd[0] = ADD(a, _mm256_set1_epi32(H[0])); w_simd[1] = ADD(b, _mm256_set1_epi32(H[1]));
    w_simd[2] = ADD(c, _mm256_set1_epi32(H[2])); w_simd[3] = ADD(d, _mm256_set1_epi32(H[3]));
    w_simd[4] = ADD(e, _mm256_set1_epi32(H[4])); w_simd[5] = ADD(f, _mm256_set1_epi32(H[5]));
    w_simd[6] = ADD(g, _mm256_set1_epi32(H[6])); w_simd[7] = ADD(h, _mm256_set1_epi32(H[7]));

    uint32_t mask = _mm256_movemask_epi8(_mm256_cmpeq_epi32(w_simd[1], _mm256_setzero_si256()));
    if (!mask) return 0xA;   // ни один lane не прошёл
    else {
        statistic[1] ++;
        mask = _mm256_movemask_epi8(_mm256_cmpeq_epi32(w_simd[0], _mm256_setzero_si256()));
        if (!mask) return 0xB;   // ни один lane не прошёл
        else {
            statistic[0] ++;
            mask = _mm256_movemask_epi8(_mm256_cmpgt_epi32(_mm256_set1_epi32(TARGET), w_simd[2]));
            if (!mask) return 0xC;   // ни один lane не прошёл
            else {
                statistic[2] ++;
                // Проверяем конкретные lane
                for (int i = 0; i < 8; i++) {
                    if (mask & (1 << (i * 4))) return i;
                }
            }
        }
    }
    return 0x9;
}

void double_sha256_scalar_w1(uint32_t r) {
    std::array<uint32_t, 16> w2;
    uint32_t a=WH[0], b=WH[1], c=WH[2], d=WH[3];
    uint32_t e=WH[4], f=WH[5], g=WH[6], h=WH[7];
    w2[0] = w1[0];
    w2[1] = w1[1];
    w2[2] = w1[2];
    w2[3] = r;
    for (uint8_t i = 4; i < 16; i ++) {
        w2[i] = w1[i];
    }

    for (uint8_t i = 0; i < 64; i++) {
        uint32_t t1 = K[i] + w2[i & 0xF] + h + V2(e) + U2(e,f,g);
        uint32_t t2 = V1(a) + U1(a,b,c);
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
        if(i < 48){
            w2[i & 0xF] += S1(w2[(i-2) & 0xF]) + S0(w2[(i+1) & 0xF]) + w2[(i-7) & 0xF];
        }
    }

    w2[0] = a + WH[0]; w2[1] = b + WH[1];
    w2[2] = c + WH[2]; w2[3] = d + WH[3];
    w2[4] = e + WH[4]; w2[5] = f + WH[5];
    w2[6] = g + WH[6]; w2[7] = h + WH[7];
    w2[8] = 0x80000000; w2[15] = 0x00000100; // 1 bit + zero <- const <=> 0x100 -> 256 bit = 32 byte <- const
    w2[10] = 0x00000000; w2[9] = 0x00000000;
    w2[12] = 0x00000000; w2[11] = 0x00000000;
    w2[14] = 0x00000000; w2[13] = 0x00000000;

    a = H[0], b = H[1], c = H[2]; d = H[3];
    e = H[4], f = H[5], g = H[6], h = H[7];

    for (uint8_t i = 0; i < 64; i++) {
        uint32_t t1 = K[i] + w2[i & 0xF] + h + V2(e) + U2(e,f,g);
        uint32_t t2 = V1(a) + U1(a,b,c);
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
        if(i < 48){
            w2[i & 0xF] += S1(w2[(i-2) & 0xF]) + S0(w2[(i+1) & 0xF]) + w2[(i-7) & 0xF];
        }
    }

    w2[0] = a + H[0]; w2[1] = b + H[1];
    w2[2] = c + H[2]; w2[3] = d + H[3];
    w2[4] = e + H[4]; w2[5] = f + H[5];
    w2[6] = g + H[6]; w2[7] = h + H[7];

    std::cout << "Hash2 " << u32_to_hex_array(w2) << std::endl;
}

void miner(int thread_id, uint32_t start_nonce) {
    v256 nonce = make_nonce_vec(start_nonce);

    for (uint32_t i = 0; i < STEP_NONCE; i ++ ) {
        
        if (nonce_found.load(std::memory_order_relaxed)) break;

        int result = double_sha256_simd(nonce);
        if (result < 0x8) {
            uint32_t valid_nonce = ((uint32_t*)&nonce)[result];
            found_nonce.store(valid_nonce, std::memory_order_relaxed);
            nonce_found.store(true, std::memory_order_relaxed);
        }
        // std::cout << thread_id << "\t" << i << "\t";
        nonce = ADD(nonce, _mm256_set1_epi32(1));
    }
}

uint32_t nonceSearch() {
    std::vector<std::thread> threads;
    uint32_t nonce;
    // prev_SHA256();
    sha256_scalar_w0();

    for (int i = 0; i < NUM_THREADS; i++) {
        uint32_t start = i * START_NONCE;
        threads.emplace_back(miner, i, start);
    }

    for (auto& t : threads)
        t.join();

    if (nonce_found.load()) {
        nonce = found_nonce.load();
        // найдено
    }
    else {
        nonce = 0;
    }

    return nonce;
}
