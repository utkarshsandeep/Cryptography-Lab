#include <stdlib.h> 
#include <string.h> 
#include <stdio.h> 
#define AES_RPOL      0x011b // reduction polynomial (x^8 + x^4 + x^3 + x + 1) 
#define AES_GEN    0x03   // gf(2^8) generator  (x + 1) 
#define AES_SBOX_CC   0x63   // S-Box C constant 
#define KEY_128 (128/8) 
#define KEY_192 (192/8) 
#define KEY_256 (256/8) 
#define aes_mul(a, b) ((a)&&(b)?g_aes_ilogt[(g_aes_logt[(a)]+g_aes_logt[(b)])%0xff]:0) 
#define aes_inv(a)  ((a)?g_aes_ilogt[0xff-g_aes_logt[(a)]]:0) 
unsigned char g_aes_logt[256], g_aes_ilogt[256]; 
unsigned char g_aes_sbox[256], g_aes_isbox[256];  
typedef struct {     
	unsigned char state[4][4];     
	int kcol;     
	size_t rounds;     
	unsigned long keysched[0]; 
} aes_ctx_t;   
void aes_init(); 
aes_ctx_t *aes_alloc_ctx(unsigned char *key, size_t keyLen); 
inline unsigned long aes_subword(unsigned long w); 
inline unsigned long aes_rotword(unsigned long w); 
void aes_keyexpansion(aes_ctx_t *ctx);  
inline unsigned char aes_mul_manual(unsigned char a, unsigned char b); // use aes_mul            instead 
void aes_subbytes(aes_ctx_t *ctx); 
void aes_shiftrows(aes_ctx_t *ctx); 
void aes_mixcolumns(aes_ctx_t *ctx); 
void aes_addroundkey(aes_ctx_t *ctx, int round); 
void aes_encrypt(aes_ctx_t *ctx, unsigned char input[16], unsigned char output[16]); 
void aes_invsubbytes(aes_ctx_t *ctx); 
void aes_invshiftrows(aes_ctx_t *ctx); 
void aes_invmixcolumns(aes_ctx_t *ctx); 
void aes_decrypt(aes_ctx_t *ctx, unsigned char input[16], unsigned char output[16]);  
void aes_free_ctx(aes_ctx_t *ctx);  
void init_aes() 
{     
	int i;     
	unsigned char gen;       
	// build logarithm table and it's inverse     
	gen = 1;     
	for(i = 0; i < 0xff; i++) {         
		g_aes_logt[gen]  = i;         
		g_aes_ilogt[i]   = gen;         
		gen = aes_mul_manual(gen, AES_GEN);     
	}       
	// build S-Box and it's inverse     
	for(i = 0; i <= 0xff; i++) {         
		char bi;         
		unsigned char inv = aes_inv(i); 
		g_aes_sbox[i] = 0;         
		for(bi = 0; bi < 8; bi++) {             
			// based on transformation 5.1             
			// could also be done with a loop based on the matrix             
			g_aes_sbox[i] |= ((inv & (1<<bi)?1:0)                         
			^ (inv & (1 << ((bi+4) & 7))?1:0)                         
			^ (inv & (1 << ((bi+5) & 7))?1:0)                         
			^ (inv & (1 << ((bi+6) & 7))?1:0)                         
			^ (inv & (1 << ((bi+7) & 7))?1:0)                         
			^ (AES_SBOX_CC & (1 << bi)?1:0)             
		) << bi;         
	}         
	g_aes_isbox[g_aes_sbox[i]] = i;     
	}     
	// warning: quickhack     
	g_aes_sbox[1] = 0x7c;     
	g_aes_isbox[0x7c] = 1;     
	g_aes_isbox[0x63] = 0; 
	}  
	aes_ctx_t *aes_alloc_ctx(unsigned char *key, size_t keyLen) {
	     aes_ctx_t *ctx;     
		 size_t rounds;     
		 size_t ks_size;     
		 switch(keyLen) {         
		 	case 16: // 128-bit key 
			rounds = 10;             
			break;         
			case 24: // 192-bit key             
			rounds = 12;             
			break;         
			case 32: // 256-bit key             
			rounds = 14;             
			break;         
			defaut:             
			return NULL;     
		}      
		ks_size = 4*(rounds+1)*sizeof(unsigned long);     
		ctx = malloc(sizeof(aes_ctx_t)+ks_size);     
		if(ctx) {
			ctx->rounds = rounds;         
			ctx->kcol = keyLen/4;         
			memcpy(ctx->keysched, key, keyLen);         
			ctx->keysched[43] = 0;         
			aes_keyexpansion(ctx);   
		}     
		return ctx; 
		} 
		inline unsigned long aes_subword(unsigned long w) 
		{     
			return g_aes_sbox[w & 0x000000ff] |         
				(g_aes_sbox[(w & 0x0000ff00) >> 8] << 8) |         
				(g_aes_sbox[(w & 0x00ff0000) >> 16] << 16) | 
				(g_aes_sbox[(w & 0xff000000) >> 24] << 24); 
		}  
		inline unsigned long aes_rotword(unsigned long w) 
		{
		     // May seem a bit different from the spec     
			 // It was changed because unsigned long is represented with little-endian convention on x86     
			 // Should not depend on architecture, but this is only a POC     
			 return ((w & 0x000000ff) << 24) |         
			 ((w & 0x0000ff00) >> 8) |         
			 ((w & 0x00ff0000) >> 8) |         
			 ((w & 0xff000000) >> 8); 
		}  
		void aes_encrypt(aes_ctx_t *ctx, unsigned char input[16], unsigned char output[16]) 
		{     
			int i;     
			// copy input to state     
			for(i = 0; i < 16; i++)         
			ctx->state[i & 0x03][i >> 2] = input[i];       
			aes_addroundkey(ctx, 0);     
			for(i = 1; i < ctx->rounds; i++) {
			         aes_subbytes(ctx);         
					 aes_shiftrows(ctx);         
					 aes_mixcolumns(ctx);         
					 aes_addroundkey(ctx, i);     
			}     
			aes_subbytes(ctx); 
			shiftrows(ctx);     
			aes_addroundkey(ctx, ctx->rounds);     
			// copy state to output     
			for(i = 0; i < 16; i++)  { 
			output[i] = ctx->state[i & 0x03][i >> 2]; 
			} 
			void aes_invshiftrows(aes_ctx_t *ctx) 
			{     
				unsigned char nstate[4][4];     
				int i;       
				for(i = 0; i < 16; i++) 
				{         
					int x, y;           
					x = i & 0x03;         
					y = i >> 2;         
					nstate[x][(y+x) & 0x03] = ctx->state[x][y];     
				}       
				memcpy(ctx->state, nstate, sizeof(ctx->state)); 
			}   
			void aes_invsubbytes(aes_ctx_t *ctx) 
			{     
				int i;     
				for(i = 0; i < 16; i++) 
				{         
					int x, y; 
					x = i & 0x03;         
					y = i >> 2;         
					ctx->state[x][y] = g_aes_isbox[ctx->state[x][y]];    
				} 
			}  
			void aes_invmixcolumns(aes_ctx_t *ctx) 
			{     
				unsigned char nstate[4][4];     
				int I;     
				for(i = 0; i < 4; i++) 
				{         
					nstate[0][i] = aes_mul(0x0e, ctx->state[0][i]) ^                 
					aes_mul(0x0b, ctx->state[1][i]) ^                 
					aes_mul(0x0d, ctx->state[2][i]) ^                 
					aes_mul(0x09, ctx->state[3][i]);         
					nstate[1][i] = aes_mul(0x09, ctx->state[0][i]) ^                 
					aes_mul(0x0e, ctx->state[1][i]) ^                 
					aes_mul(0x0b, ctx->state[2][i]) ^                 
					aes_mul(0x0d, ctx->state[3][i]);         
					nstate[2][i] = aes_mul(0x0d, ctx->state[0][i]) ^                 
					aes_mul(0x09, ctx->state[1][i]) ^                 
					aes_mul(0x0e, ctx->state[2][i]) ^                 
					aes_mul(0x0b, ctx->state[3][i]);         
					nstate[3][i] = aes_mul(0x0b, ctx->state[0][i]) ^                 
					aes_mul(0x0d, ctx->state[1][i]) ^                 
					aes_mul(0x09, ctx->state[2][i]) ^                 
					aes_mul(0x0e, ctx->state[3][i]); 
				}       
				memcpy(ctx->state, nstate, sizeof(ctx->state)); 
			}   
			void aes_decrypt(aes_ctx_t *ctx, unsigned char input[16], unsigned char output[16]) 
			{     
				int i, j;       
				// copy input to state     
				for(i = 0; i < 16; i++)         
					ctx->state[i & 0x03][i >> 2] = input[i];       
				aes_addroundkey(ctx, ctx->rounds);     
				for(i = ctx->rounds-1; i >= 1; i--) {         
					aes_invshiftrows(ctx);         
					aes_invsubbytes(ctx);         
					aes_addroundkey(ctx, i);         
					aes_invmixcolumns(ctx);     
				}       
				aes_invshiftrows(ctx);     
				aes_invsubbytes(ctx);     
				aes_addroundkey(ctx, 0);       
				// copy state to output     
				for(i = 0; i < 16; i++) 
				{
					output[i] = ctx->state[i & 0x03][i >> 2]; 
				}   
				void aes_free_ctx(aes_ctx_t *ctx) 
				{     
					free(ctx); 
				}   
				int main(int argc, char *argv[]) 
				{     
					unsigned char key[KEY_128] = "uber strong key!";     
					unsigned char ptext[16] = "Attack at dawn!";     
					unsigned char ctext[16];     
					unsigned char decptext[16];     
					aes_ctx_t *ctx;     
					init_aes();     
					ctx = aes_alloc_ctx(key, sizeof(key));     
					if(!ctx) 
					{         
						perror("aes_alloc_ctx");         
						return EXIT_FAILURE;     
					}     
					aes_encrypt(ctx, ptext, ctext);     
					aes_decrypt(ctx, ctext, decptext);     
					puts(decptext);
					aes_free_ctx(ctx);     
					return EXIT_SUCCESS;
				}
			}
		}
