/*
  Normil Luccin
  Z23635489
  Assignment 5
*/

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define EX1 1
#define EX2 1
#define EX3 1
#define EX4 1
#define EX5 1
#define EX6 1
#define EX7 1

#define REAL_RANDOM 0

FILE *fbank;

#if REAL_RANDOM
#define RANDOM_DATA(ptr, size) \
FILE *frandom = fopen("/dev/urandom", "r"); \
fread((ptr), (size), 1, frandom); \
fclose(frandom);
#else
#define RANDOM_DATA(ptr, size) \
fread((ptr), (size), 1, fbank);
#endif // REAL_RANDOM


typedef uint64_t bigint256[16]; //for operands
typedef uint64_t bigint512[32]; //for multiplication result

const bigint256 PRIME          = {0xffed,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0x7fff};

// a point struct contains two bigint192 pointers x and y
typedef struct{
    bigint256	x;
    bigint256	y;
} point;

// // The following function get the time-stamp from processor, only works on Unix machines
// int64_t cpucycles(void) {
//     unsigned int hi, lo;

//     asm volatile ("rdtsc\n\t" : "=a" (lo), "=d"(hi));
//     return ((int64_t)lo) | (((int64_t)hi) << 32);
// }

// Print 256 bit number as 1 hex integer (Helper)
void print_hex256(const char *s, const bigint256 a) {
    int i;
    printf("%s", s);
    for(i = 15; i >= 0; i--)
        printf("%04lx",a[i]);
    printf("\n");
}

// Read a 256-bit hex integer into bigint256 (Helper). 1 indicates output valid. 0 not valid.
int read_hex256(const char *s, const char* a, bigint256 r) {
    static const long hextable[] = { 
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1, 0,1,2,3,4,5,6,7,8,9,-1,-1,-1,-1,-1,-1,-1,10,11,12,13,14,15,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,10,11,12,13,14,15,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
    };
    
    int i,j;
    uint8_t c;
    long hex_value;
    
    if(strlen(a) != 64) {
        printf("Error: Length of %s is (%ld). It should be 64.\n", s, strlen(a));
        return 0;
    }

    memset(r, 0, sizeof(bigint256));

    for(i = 15; i>=0; i--) {
        for (j = 3; j >= 0; j--) {
            c = *a++;
            hex_value = hextable[c];
            if (hex_value == -1) {
                printf("Error: Found invalid hex digit '%c'.\n", c);
                return 0;
            }
            r[i] += hex_value << (4*j);
        }
    }
    return 1;
}

// Print 256 bit number (Helper)
void bigint256_print(const bigint256 a) {
    int i;
    printf("{");
    for(i = 0; i < 15; i++)
        printf("0x%04lx,",a[i]);
    printf("0x%04lx}",a[i]);
}

//print 512 bit number (Helper)
void bigint512_print(const bigint512 a) {
    int i;
    printf("{");
    for(i = 0; i < 8; i++)
        printf("0x%09lx,",a[i]);
    printf("\n");
    for(i = 8; i < 16; i++)
        printf("0x%09lx,",a[i]);
    printf("\n");
    for(i = 16; i < 24; i++)
        printf("0x%09lx,",a[i]);
    printf("\n");
    for(i = 24; i < 31; i++)
        printf("0x%09lx,",a[i]);
    printf("0x%09lx}",a[i]);
}

// Print point (Helper)
void point_print(const point *P) {
    int i;
    printf("\nx={");
    for(i = 0; i < 15; i++)
        printf("0x%04lx,",P->x[i]);
    printf("0x%04lx}",P->x[i]);
    printf("\ny={");
    for(i = 0; i < 15; i++)
        printf("0x%04lx,",P->y[i]);
    printf("0x%04lx}",P->y[i]);
}


//##########################################
//# Fp Arithmetic
//##########################################
// r = (a + b) % PRIME
void mod_add(bigint256 r, const bigint256 a, const bigint256 b) {
    int i;
    uint64_t carry;
    uint64_t mask;

    // r = a+b
    carry = 0;
    for(i=0; i<15; i++) {
        r[i] = a[i] + b[i] + carry;
        carry = r[i] >> 16;
        r[i] &= 0xffff;
    }
    r[i] = a[i] + b[i] + carry;

    // r = r - PRIME
    carry = 0;
    for(i=0; i<15; i++) {
        r[i] = r[i] - PRIME[i] - carry;
        carry = r[i] >> 63;
        r[i] &= 0xffff;
    }
    r[i] = r[i] - PRIME[i] - carry;

    // Get the mask from sign
    mask = 0 - (r[15] >> 63);

    // r = r + (mask & PRIME)
    carry = 0;
    for(i=0; i<15; i++) {
        r[i] = r[i] + (mask & PRIME[i]) + carry;
        carry = r[i] >> 16;
        r[i] &= 0xffff;
    }
    r[i] = r[i] + (mask & PRIME[i]) + carry;
}

// r = (a - b) % PRIME
void mod_sub(bigint256 r, const bigint256 a, const bigint256 b) {
    int i;
    uint64_t carry;
    uint64_t mask;

    // r = a-b
    carry = 0;
    for(i=0; i<15; i++) {
        r[i] = a[i] - b[i] - carry;
        carry = r[i] >> 63;
        r[i] &= 0xffff;
    }
    r[i] = a[i] - b[i] - carry;

    // Get the mask from sign
    mask = 0 - (r[15] >> 63);

    // r = r + (mask & PRIME)
    carry = 0;
    for(i=0; i<15; i++) {
        r[i] = r[i] + (mask & PRIME[i]) + carry;
        carry = r[i] >> 16;
        r[i] &= 0xffff;
    }
    r[i] = r[i] + (mask & PRIME[i]) + carry;
}

// r = a * b using Karatsuba method
void mul(bigint512 r, const bigint256 a, const bigint256 b) {
    int i, j;

    //initialize arrays
    uint64_t ma[8];
    uint64_t mb[8];
    uint64_t z0[16] = {0};
    uint64_t z1[16] = {0};
    uint64_t z2[16] = {0};

    // Write 3d here
    // Compute ma, mb
    for(i = 0; i < 8; i++) {
        ma[i] = a[i] + a[8+i];
        mb[i] = b[i] + b[8+i];
    }

    // Compute z0, z1, z2
    for(i = 0; i < 8; i++) {
        for(j = 0; j < 8; j++) {
            z0[i+j] += a[i] * b[j];
            z1[i+j] += ma[i] * mb[j];
            z2[i+j] += a[8+i] * b[8+j];
        }
    }

    // Perform subtraction z1 = z1 - z0 - z2
    for(i = 0; i < 15; i++) // z0[15],z1[15],z2[15] are 0
        z1[i] = z1[i] - z0[i] - z2[i];


    // Implement final addition and put the result in r
    for(i = 0; i < 8; i++) {
        r[i] = z0[i];
        r[8+i] = z0[8+i] + z1[i];
        r[16+i] = z1[8+i] + z2[i];
        r[24+i] = z2[8+i];
    }
}

// r = a % PRIME using pseudo mersenne reduction
void psu_reduce(bigint256 r, const bigint512 a) {
    int i;
    uint64_t carry, mask;

    // First round of pseudo-mersenne with carry propagation
    // a[0-15] + 38*a[16-31]
    carry = 0;
    for(i = 0; i < 15; i++) {
        r[i] = a[i] + 38*a[16+i] + carry;
        carry = r[i] >> 16;
        r[i] &= 0xffff;
    }
    r[i] = a[i] + 38*a[16+i] + carry;

    // Second round of pseudo-mersenne 
    r[0] = r[0] + 19*(r[15]>>15);
    r[15] &= 0x7fff; // Remove the high bits.

    // Perform carry propagation
    carry = 0;
    for(i = 0; i < 15; i++) {
        r[i] = r[i] + carry;
        carry = r[i] >> 16;
        r[i] &= 0xffff;
    }
    r[i] = r[i] + carry;

    // r = r mod p
    carry = 0;
    for(i=0; i<15; i++) {
        r[i] = r[i]- PRIME[i] - carry;
        carry = r[i] >> 63;
        r[i] &= 0xffff;
    }
    r[i] = r[i] - PRIME[i] - carry;

    mask = 0 - (r[15] >> 63);

    carry = 0;
    for(i=0; i<15; i++) {
        r[i] = r[i] + (mask & PRIME[i]) + carry;
        carry = r[i] >> 16;
        r[i] &= 0xffff;
    }
    r[i] = r[i] + (mask & PRIME[i]) + carry;
}

void mod_mul(bigint256 r, const bigint256 a, const bigint256 b){
    bigint512 t;
    mul(t, a, b);
    psu_reduce(r, t);
}

void mod_sqr(bigint256 r, const bigint256 a){
    bigint512 t;
    mul(t, a, a);
    psu_reduce(r, t);
}

// r = (a^-1) % 2PRIME using pseudo mersenne reduction
void mod_inv(bigint256 r, const bigint256 a) {
    int i;
    bigint256 t;

    // Do NOT write into r until the very end. Otherwise, if you pass mod_inv(b,b) for example, it will output incorrect result. Use t as temporary variable.
    memcpy(t, a, sizeof(bigint256));
    
    // 249 1s (first one is the initialization)
    for(i = 0; i < 249; i++) {
        mod_sqr(t, t);
        mod_mul(t, t, a);
    }
    // 0
    mod_sqr(t, t);
    // 1
    mod_sqr(t, t);
    mod_mul(t, t, a);
    // 0
    mod_sqr(t, t);
    // 1
    mod_sqr(t, t);
    mod_mul(t, t, a);
    // 1
    mod_sqr(t, t);
    mod_mul(r, t, a); // Write into r in the last one.
}

//##########################################
//# Point arithmetic
//##########################################
// R = P + Q
void point_add(point *R, const point *P, const point *Q) {
    bigint256 t;
    bigint256 m, xR, yR;

    // Note to access x and y coordinate of P and Q use P->x, P->y, Q->x, Q->y
    
    // Write 1 here
    // slope m
    mod_sub(m, Q->y, P->y); // a
    mod_sub(t, Q->x, P->x); // b
    mod_inv(t, t); // b^-1 = 1/b 
    mod_mul(m, m, t);
    

    // xR
    mod_sqr(xR, m);
    mod_sub(xR, xR, P->x);
    mod_sub(xR, xR, Q->x);

    // yR
    mod_sub(yR, P->x, xR);
    mod_mul(yR, m, yR);
    mod_sub(yR, yR, P->y);


    //R = (xR,yR)
    memcpy(R->x, xR, sizeof(bigint256));
    memcpy(R->y, yR, sizeof(bigint256));
}

// R = P + P
void point_double(point *R, const point *P) {
    const bigint256 a = {0xa144,0x4914,0xaa98,0xaaaa,0xaaaa,0xaaaa,0xaaaa,0xaaaa,0xaaaa,0xaaaa,0xaaaa,0xaaaa,0xaaaa,0xaaaa,0xaaaa,0x2aaa};
    bigint256 t;
    bigint256 m, xR, yR;

    // Note to access x and y coordinate of P use P->x, P->y
    
    // Write 2 here
    // slope m
    mod_sqr(t, P->x);
    mod_add(m, t, t);
    mod_add(m, m, t);
  
    mod_add(m, m, a);

    mod_add(t, P->y, P->y);
    mod_inv(t, t);
    mod_mul(m, m, t);
    

    

    // xR
    mod_sqr(xR, m);
    mod_sub(xR, xR, P->x);
    mod_sub(xR, xR, P->x);

    // yR
    mod_sub(yR, P->x, xR);
    mod_mul(yR, m, yR);
    mod_sub(yR, yR, P->y);

    // R = (xR,yR)
    memcpy(R->x, xR, sizeof(bigint256));
    memcpy(R->y, yR, sizeof(bigint256));
}


// If option == 0, then R = P. If option == 1 then R = Q.
void select_point(point *R, const point *P, const point *Q, uint8_t option) {
    int i;
    uint64_t t;
    uint64_t mask;

    // Note to access x and y coordinate of P, Q and R use P->x, P->y, Q->x, Q->y, R->x,     R->y

    // Write 3 here
    mask = 0 - option;
    for(i=0; i < 16; i++){
      t = P->x[i] ^ Q->x[i];
      t &= mask;
      R->x[i] = P->x[i] ^ t;

      t = P->y[i] ^ Q->y[i];
      t &= mask;
      R->y[i] = P->y[i] ^ t;
    }


}

// xR where R = [s]P
void point_mul(bigint256 xR, const point *P, const bigint256 s) {
    int i, j;
    uint8_t bit;
	point T, T2; // Use &T for pointer. Access coordinates using T.x and T.y

    // Write 4 here
    // Implement using "Double-and-add technique" technique
    memcpy(&T, P, sizeof(point));

    // ???
    // first we process bits 0...13 in index 15
    for(i = 13; i >=0; i--){
      bit = (s[15] >> i) & 1;
      point_double(&T, &T);
      point_add(&T2, &T, P);
      select_point(&T, &T, &T2, bit);
    }
    // second we process bits 0...15 in indexes 14-0
    for(i = 14; i >=0; i--){
      for(j = 15; j >=0; j--){
        bit = (s[i] >> j) & 1;
        point_double(&T, &T);
        point_add(&T2, &T, P);
        select_point(&T, &T, &T2, bit);
      }
    }
    
    memcpy(xR, T.x, sizeof(bigint256));


}

// Compute y from x for E:y^2=x^3+ax+b
void recover_y(bigint256 y, const bigint256 x) {
    const bigint256 a = {0xa144,0x4914,0xaa98,0xaaaa,0xaaaa,0xaaaa,0xaaaa,0xaaaa,0xaaaa,0xaaaa,0xaaaa,0xaaaa,0xaaaa,0xaaaa,0xaaaa,0x2aaa};
    const bigint256 b = {0xc864,0x7710,0x5e9c,0x260b,0x97b4,0x5ed0,0x7b42,0xed09,0xb425,0xd097,0x425e,0x097b,0x25ed,0x97b4,0x5ed0,0x7b42};
    int i;
    bigint256 t;
    bigint256 s, u, v, w;

    // Write 5 here
    // Compute s=x^3+ax+b
    mod_sqr(t, x);
    mod_mul(s, t, x);
    mod_mul(t, a, x);
    mod_add(s, s, t);
    mod_add(s, s, b);
    
    // Compute u = (2*s)
    mod_add(u, s, s);


    // Compute v = u^((p-5)/8)
    memcpy(v, u, sizeof(bigint256));




    // Compute w = u*v^2
    mod_sqr(t, v);
    mod_mul(w,t,u);

    // Compute y = s*v*(w-1) = s*v*w - s*v
    mod_mul(t, s, v);
    mod_mul(y, t, w);
    mod_mul(t, s, v);
    mod_sub(y, y, t);



}


//##########################################
//# ECDH
//##########################################
// Generate secret key and public key.
void keyGen(bigint256 sk, bigint256 pk) {
    // Generator point (Use &G to use it as a pointer, use G.x and G.y to access x and y coordinates)
    const point G= {
        .x={0x245a,0xaaad,0xaaaa,0xaaaa,0xaaaa,0xaaaa,0xaaaa,0xaaaa,0xaaaa,0xaaaa,0xaaaa,0xaaaa,0xaaaa,0xaaaa,0xaaaa,0x2aaa},
        .y={0x2c14,0x8131,0x3a5d,0xd616,0x9e4d,0x9283,0xb281,0x6dc2,0x2eb3,0x88b7,0x22d3,0x1fe1,0x794b,0x475f,0xe65e,0x5f51}
    };
    int i;

    // Write 6 here
    // Generate secret key here (first line is given to you)
    RANDOM_DATA(sk, sizeof(bigint256)); // This will read from the file once and fill sk with random data.

    for(i = 0; i < 16; i++){
      sk[i] &= 0xFFFF;
    }
    // Generate public key here
    sk[0] &= 0xFFF8;
    sk[15] &= 0x7FFF; 
    sk[15] |= 0x4000;
    point_mul(pk, &G, sk);
    
  
    
}

// Generate shared secret from your secret key and other party's public key
void sharedSecret(bigint256 ss, const bigint256 sk, const bigint256 pk) {
    point T; // Use &T for pointer. Access x and y coordinates using T.x and T.y

    memcpy(T.x, pk, sizeof(bigint256)); // Copy pk to x-coordinate
    
    // Write 7 here
    // Regenerate y-coordinate of T here
    recover_y(T.y, T.x);

    // Compute shared secret here
    point_mul(ss, pk, sk);
    
}

int main(int argc, char* argv[]) {
#if REAL_RANDOM
    int rtn;
    bigint256 sk;
    bigint256 pk;
    bigint256 ss;

    if (argc < 2) {
        printf("Arguments are required:\n");
        printf("For keygen: pass 'keygen'\n");
        printf("For shared: pass 'shared sk pk' where sk and pk are secret and public key in hex, respectively\n");
        return 1;
    }

    if(!strcmp(argv[1], "keygen")) {
        keyGen(sk, pk);
        print_hex256("sk = ", sk);
        print_hex256("pk = ", pk);
    } else if(!strcmp(argv[1], "shared")) {
        if(argc < 4) {
            printf("2 additional arguments are required:\n");
            printf("'sk pk' where sk and pk are secret and public key in hex, respectively\n");
            return 1;
        }
        rtn = read_hex256("sk", argv[2], sk);
        if (!rtn) {
            printf("sk = %s", argv[2]);
            return 1;
        }
        rtn = read_hex256("pk", argv[3], pk);
        if (!rtn) {
            printf("pk = %s", argv[3]);
            return 1;
        }

        sharedSecret(ss ,sk, pk);
        print_hex256("sk = ", sk);
        print_hex256("pk = ", pk);
        print_hex256("ss = ", ss);
    } else {
        printf("Invalid Argument '%s' : \n", argv[1]);
        printf("For keygen: pass 'keygen'\n");
        printf("For shared: pass 'shared sk pk' where sk and pk are secret and public key in hex, respectively\n");
        return 1;
    }
#else
    const int BANK_TOTAL = 100;
    int i, pass;

    point a, b;
    bigint256 x, s;
    uint8_t option = 0;
    point exp_pt, act_pt;
    bigint256 exp_scalar1, exp_scalar2, act_scalar;

    bigint256 exp_sk, exp_pk, exp_ss;
    bigint256 act_sk, act_pk, act_ss;

    // unsigned long long cycles, cycles1, cycles2;

    fbank = fopen("assignment_5a_bank","r");

    if (fbank == NULL) {
        printf("'assignment_5a_bank' file is missing. Please copy it to the same directory as where you execute the code.\n");
        return 1;
    }

#if EX1
    printf("###############################\n");
    printf("Exercise 1:\n");
    printf("###############################\n");
    rewind(fbank);
    pass = 1;
    // cycles = 0;
    for (i = 0; i < BANK_TOTAL; i++) {
        fread(&a, sizeof(point), 1, fbank);
        fread(&b, sizeof(point), 1, fbank);
        fread(&exp_pt, sizeof(point), 1, fbank);

        // cycles1 = cpucycles();
        point_add(&act_pt, &a, &b);
        // cycles2 = cpucycles();
        // cycles = cycles + (cycles2 - cycles1);
        if( memcmp(act_pt.x, exp_pt.x, sizeof(bigint256)) || memcmp(act_pt.y, exp_pt.y, sizeof(bigint256)) ) {
            printf("point_add failed for these values:\n");
            printf("P   = "); point_print(&a); printf("\n");
            printf("Q   = "); point_print(&b); printf("\n");
            printf("exp = "); point_print(&exp_pt); printf("\n");
            printf("act = "); point_print(&act_pt); printf("\n");
            pass = 0;
            break;
        }
    }

    if(pass == 1) {
        printf("point_add passed!\n");
        // printf("point_add runs in %10llu cycles\n", cycles/BANK_TOTAL);
    }
    printf("###############################\n\n");
#endif

#if EX2
    printf("###############################\n");
    printf("Exercise 2:\n");
    printf("###############################\n");
    rewind(fbank);
    fseek(fbank, 3*sizeof(point)*BANK_TOTAL,SEEK_SET);
    pass = 1;
    // cycles = 0;
    for (i = 0; i < BANK_TOTAL; i++) {
        fread(&a, sizeof(point), 1, fbank);
        fread(&exp_pt, sizeof(point), 1, fbank);

        // cycles1 = cpucycles();
        point_double(&act_pt, &a);
        // cycles2 = cpucycles();
        // cycles = cycles + (cycles2 - cycles1);
        if( memcmp(act_pt.x, exp_pt.x, sizeof(bigint256)) || memcmp(act_pt.y, exp_pt.y, sizeof(bigint256)) ) {
            printf("i=%d\n", i);
            printf("point_double failed for these values:\n");
            printf("P   = "); point_print(&a); printf("\n");
            printf("exp = "); point_print(&exp_pt); printf("\n");
            printf("act = "); point_print(&act_pt); printf("\n");
            pass = 0;
            break;
        }
    }

    if(pass == 1) {
        printf("point_double passed!\n");
        // printf("point_double runs in %10llu cycles\n", cycles/BANK_TOTAL);
    }
    printf("###############################\n\n");
#endif

#if EX3
    printf("###############################\n");
    printf("Exercise 3:\n");
    printf("###############################\n");
    rewind(fbank);
    fseek(fbank, 5*sizeof(point)*BANK_TOTAL,SEEK_SET);
    pass = 1;
    // cycles = 0;
    for (i = 0; i < BANK_TOTAL; i++) {
        fread(&a, sizeof(point), 1, fbank);
        fread(&b, sizeof(point), 1, fbank);
        fread(&option, 1, 1, fbank);
        fread(&exp_pt, sizeof(point), 1, fbank);

        // cycles1 = cpucycles();
        select_point(&act_pt, &a, &b, option);
        // cycles2 = cpucycles();
        // cycles = cycles + (cycles2 - cycles1);
        if(memcmp(&act_pt, &exp_pt, sizeof(point)) ) {
            printf("select_point failed for these values:\n");
            printf("a   = "); point_print(&a); printf("\n");
            printf("b   = "); point_print(&b); printf("\n");
            printf("option = %d\n", option);
            printf("exp = "); point_print(&exp_pt); printf("\n");
            printf("act = "); point_print(&act_pt); printf("\n");
            pass = 0;
            break;
        }
    }

    if(pass == 1) {
        printf("select_point passed!\n");
        // printf("select_point runs in %10llu cycles\n", cycles/BANK_TOTAL);
    }
    printf("###############################\n\n");
#endif

#if EX4
    printf("###############################\n");
    printf("Exercise 4:\n");
    printf("###############################\n");
    rewind(fbank);
    fseek(fbank, (8*sizeof(point)+1)*BANK_TOTAL,SEEK_SET);
    pass = 1;
    // cycles = 0;
    for (i = 0; i < BANK_TOTAL; i++) {
        fread(&a, sizeof(point), 1, fbank);
        fread(s, sizeof(bigint256), 1, fbank);
        fread(exp_scalar1, sizeof(bigint256), 1, fbank);

        // cycles1 = cpucycles();
        point_mul(act_scalar, &a, s);
        // cycles2 = cpucycles();
        // cycles = cycles + (cycles2 - cycles1);
        if(memcmp(act_scalar, exp_scalar1, sizeof(bigint256))) {
            printf("point_mul failed for these values:\n");
            printf("a   = "); point_print(&a); printf("\n");
            printf("b   = "); bigint256_print(s); printf("\n");
            printf("exp = "); bigint256_print(exp_scalar1); printf("\n");
            printf("res = "); bigint256_print(act_scalar); printf("\n");
            pass = 0;
            break;
        }
    }

    if(pass == 1) {
        printf("point_mul passed!\n");
        // printf("point_mul runs in %10llu cycles\n", cycles/BANK_TOTAL);
    }
    printf("###############################\n\n");
#endif

#if EX5
    printf("###############################\n");
    printf("Exercise 5:\n");
    printf("###############################\n");
    rewind(fbank);
    fseek(fbank, (9*sizeof(point)+2*sizeof(bigint256)+1)*BANK_TOTAL,SEEK_SET);
    pass = 1;
    // cycles = 0;
    for (i = 0; i < BANK_TOTAL; i++) {
        fread(x, sizeof(bigint256), 1, fbank);
        fread(exp_scalar1, sizeof(bigint256), 1, fbank);
        fread(exp_scalar2, sizeof(bigint256), 1, fbank);

        // cycles1 = cpucycles();
        recover_y(act_scalar, x);
        // cycles2 = cpucycles();
        // cycles = cycles + (cycles2 - cycles1);
        if( memcmp(act_scalar, exp_scalar1, sizeof(bigint256)) && memcmp(act_scalar, exp_scalar2, sizeof(bigint256)) ) {
            printf("recover_y failed for these values:\n");
            printf("x     = "); bigint256_print(x); printf("\n");
            printf("y_pos = "); bigint256_print(exp_scalar1); printf("\n");
            printf("y_neg = "); bigint256_print(exp_scalar2); printf("\n");
            printf("res   = "); bigint256_print(act_scalar); printf("\n");
            pass = 0;
            break;
        }
    }

    if(pass == 1) {
        printf("recover_y passed!\n");
        // printf("recover_y runs in %10llu cycles\n", cycles/BANK_TOTAL);
    }
    printf("###############################\n\n");
#endif

#if EX6
    printf("###############################\n");
    printf("Exercise 6:\n");
    printf("###############################\n");
    rewind(fbank);
    fseek(fbank, (9*sizeof(point)+5*sizeof(bigint256)+1)*BANK_TOTAL,SEEK_SET);
    pass = 1;
    // cycles = 0;
    for (i = 0; i < BANK_TOTAL; i++) {
        fread(exp_sk, sizeof(bigint256), 1, fbank);
        fread(exp_pk, sizeof(bigint256), 1, fbank);

        // cycles1 = cpucycles();
        keyGen(act_sk, act_pk);
        // cycles2 = cpucycles();
        // cycles = cycles + (cycles2 - cycles1);
        if(memcmp(exp_sk, act_sk, sizeof(bigint256)) || memcmp(exp_pk, act_pk, sizeof(bigint256))) {
            printf("keyGen failed for these values:\n");
            printf("exp_sk   = "); bigint256_print(exp_sk); printf("\n");
            printf("exp_pk   = "); bigint256_print(exp_pk); printf("\n");
            printf("act_sk   = "); bigint256_print(act_sk); printf("\n");
            printf("act_pk   = "); bigint256_print(act_pk); printf("\n");
            pass = 0;
            break;
        }
    }

    if(pass == 1) {
        printf("keyGen passed!\n");
        // printf("keyGen runs in %10llu cycles\n", cycles/BANK_TOTAL);
    }
    printf("###############################\n\n");
#endif

#if EX7
    printf("###############################\n");
    printf("Exercise 7:\n");
    printf("###############################\n");
    rewind(fbank);
    fseek(fbank, (9*sizeof(point)+8*sizeof(bigint256)+1)*BANK_TOTAL,SEEK_SET);
    pass = 1;
    // cycles = 0;
    for (i = 0; i < BANK_TOTAL; i++) {
        fread(exp_sk, sizeof(bigint256), 1, fbank);
        fread(exp_pk, sizeof(bigint256), 1, fbank);
        fread(exp_ss, sizeof(bigint256), 1, fbank);

        // cycles1 = cpucycles();
        sharedSecret(act_ss, exp_sk, exp_pk);
        // cycles2 = cpucycles();
        // cycles = cycles + (cycles2 - cycles1);
        if(memcmp(exp_ss, act_ss, sizeof(bigint256))) {
            printf("sharedSecret failed for these values:\n");
            printf("sk       = "); bigint256_print(exp_sk); printf("\n");
            printf("pk       = "); bigint256_print(exp_pk); printf("\n");
            printf("exp_ss   = "); bigint256_print(exp_ss); printf("\n");
            printf("act_ss   = "); bigint256_print(act_ss); printf("\n");
            pass = 0;
            break;
        }
    }

    if(pass == 1) {
        printf("sharedSecret passed!\n");
        // printf("sharedSecret runs in %10llu cycles\n", cycles/BANK_TOTAL);
    }
    printf("###############################\n\n");
#endif

    fclose(fbank);
#endif // REAL_RANDOM

    return 0;
}

/* Output
  2d: Eve can create her own public key since she knows both Alice and Bob's public key. So she replaces the actual public key with the public key of Eve        (pkEA,pkEB) and sends those forged public keys to Alice and Bob, practically gaining access to Alice's and Bob's shared secrets.

  2e:

*/
