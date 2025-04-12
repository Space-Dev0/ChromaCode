#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <stdint.h>
#include <string.h>
#include <vector>
#include <iostream>
#include "pseudoRandom.hpp"
#include "ldpc.hpp"

#define reportError(a) std::cout<<a<<'\n';exit(15);

std::vector<int32_t> createMatrixA(int32_t wc, int32_t wr, int32_t capacity) {
  int32_t nb_pcb;
  if (wr < 4)
    nb_pcb = capacity / 2;
  else
    nb_pcb = capacity / wr * wc;

  int32_t effwidth = ceil(capacity / (float_t)32) * 32;
  int32_t offset = ceil(capacity / (float_t)32);
  std::vector<int32_t> matrixA(ceil(capacity / (float_t)32 * nb_pcb * 4));
  std::vector<int32_t> permutation(capacity * 4);

  for (int32_t i = 0; i < capacity; i++) {
    permutation[i] = i;
  }

  for (int32_t i = 0; i < capacity / wr; i++) {
    for (int32_t j = 0; j < wr; j++) {
      matrixA[(i * (effwidth + wr) + j) / 32] =
          1 << (31 - ((i * (effwidth + wr) + j) % 32));
    }
  }

  setSeed(LPDC_MESSAGE_SEED);
  for (int32_t i = 1; i < wc; i++) {
    int32_t off_index = i * (capacity / wr);
    for (int32_t j = 0; j < capacity; j++) {
      int32_t pos = (int32_t)((float_t)lcg64_temper() /
                                  (float_t)UINT32_MAX * (capacity - j));
      for (int32_t k = 0; k < capacity / wr; k++)
        matrixA[(off_index + k) * offset + j / 32] |=
            ((matrixA[(permutation[pos] / 32 + k * offset)] >>
              (31 - permutation[pos] % 32)) &
             1)
            << (31 - j % 32);
      int32_t tmp = permutation[capacity - 1 - j];
      permutation[capacity - 1 - j] = permutation[pos];
      permutation[pos] = tmp;
    }
  }
  return matrixA;
}
  int32_t GaussJordan(int32_t* matrixA, int32_t wc, int32_t wr, int32_t capacity, int32_t* matrix_rank, bool encode)
{
    int32_t loop=0;
    int32_t nb_pcb;
    if(wr<4)
        nb_pcb=capacity/2;
    else
        nb_pcb=capacity/wr*wc;

    int32_t offset=ceil(capacity/(float)32);

    int32_t*matrixH=(int32_t *)calloc(offset*nb_pcb,sizeof(int32_t));
    if(matrixH == NULL)
    {
        reportError("Memory allocation for matrix in LDPC failed");
        return 1;
    }
    memcpy(matrixH,matrixA,offset*nb_pcb*sizeof(int32_t));

    int32_t* column_arrangement=(int32_t *)calloc(capacity, sizeof(int32_t));
    if(column_arrangement == NULL)
    {
        reportError("Memory allocation for matrix in LDPC failed");
        free(matrixH);
        return 1;
    }
    bool* processed_column=(bool *)calloc(capacity, sizeof(bool));
    if(processed_column == NULL)
    {
        reportError("Memory allocation for matrix in LDPC failed");
        free(matrixH);
        free(column_arrangement);
        return 1;
    }
    int32_t* zero_lines_nb=(int32_t *)calloc(nb_pcb, sizeof(int32_t));
    if(zero_lines_nb == NULL)
    {
        reportError("Memory allocation for matrix in LDPC failed");
        free(matrixH);
        free(column_arrangement);
        free(processed_column);
        return 1;
    }
    int32_t* swap_col=(int32_t *)calloc(2*capacity, sizeof(int32_t));
    if(swap_col == NULL)
    {
        reportError("Memory allocation for matrix in LDPC failed");
        free(matrixH);
        free(column_arrangement);
        free(processed_column);
        free(zero_lines_nb);
        return 1;
    }

    int32_t zero_lines=0;

    for (int32_t i=0; i<nb_pcb; i++)
    {
        int32_t pivot_column=capacity+1;
        for (int32_t j=0; j<capacity; j++)
        {
            if((matrixH[(offset*32*i+j)/32] >> (31-(offset*32*i+j)%32)) & 1)
            {
                pivot_column=j;
                break;
            }
        }
        if(pivot_column < capacity)
        {
            processed_column[pivot_column]=1;
            column_arrangement[pivot_column]=i;
            if (pivot_column>=nb_pcb)
            {
                swap_col[2*loop]=pivot_column;
                loop++;
            }

            int32_t off_index=pivot_column/32;
            int32_t off_index1=pivot_column%32;
            for (int32_t j=0; j<nb_pcb; j++)
            {
                if (((matrixH[off_index+j*offset] >> (31-off_index1)) & 1) && j != i)
                {
                    //subtract pivot row GF(2)
                    for (int32_t k=0;k<offset;k++)
                        matrixH[k+offset*j] ^= matrixH[k+offset*i];
                }
            }
        }
        else //zero line
        {
            zero_lines_nb[zero_lines]=i;
            zero_lines++;
        }
    }

    *matrix_rank=nb_pcb-zero_lines;
    int32_t loop2=0;
    for(int32_t i=*matrix_rank;i<nb_pcb;i++)
    {
        if(column_arrangement[i] > 0)
        {
            for (int32_t j=0;j < nb_pcb;j++)
            {
                if (processed_column[j] == 0)
                {
                    column_arrangement[j]=column_arrangement[i];
                    column_arrangement[i]=0;
                    processed_column[j]=1;
                    processed_column[i]=0;
                    swap_col[2*loop]=i;
                    swap_col[2*loop+1]=j;
                    column_arrangement[i]=j;
                    loop++;
                    loop2++;
                    break;
                }
            }
        }
    }

    int32_t loop1=0;
    for (int32_t kl=0;kl< nb_pcb;kl++)
    {
        if(processed_column[kl] == 0 && loop1 < loop-loop2)
        {
            column_arrangement[kl]=column_arrangement[swap_col[2*loop1]];
            processed_column[kl]=1;
            swap_col[2*loop1+1]=kl;
            loop1++;
        }
    }

    loop1=0;
    for (int32_t kl=0;kl< nb_pcb;kl++)
    {
        if(processed_column[kl]==0)
        {
            column_arrangement[kl]=zero_lines_nb[loop1];
            loop1++;
        }
    }
    //rearrange matrixH if encoder and store it in matrixA
    //rearrange matrixA if decoder
    if(encode)
    {
        for(int32_t i=0;i< nb_pcb;i++)
            memcpy(matrixA+i*offset,matrixH+column_arrangement[i]*offset,offset*sizeof(int32_t));

        //swap columns
        int32_t tmp=0;
        for(int32_t i=0;i<loop;i++)
        {
            for (int32_t j=0;j<nb_pcb;j++)
            {
                tmp ^= (-((matrixA[swap_col[2*i]/32+j*offset] >> (31-swap_col[2*i]%32)) & 1) ^ tmp) & (1 << 0);
                matrixA[swap_col[2*i]/32+j*offset]   ^= (-((matrixA[swap_col[2*i+1]/32+j*offset] >> (31-swap_col[2*i+1]%32)) & 1) ^ matrixA[swap_col[2*i]/32+j*offset]) & (1 << (31-swap_col[2*i]%32));
                matrixA[swap_col[2*i+1]/32+offset*j] ^= (-((tmp >> 0) & 1) ^ matrixA[swap_col[2*i+1]/32+offset*j]) & (1 << (31-swap_col[2*i+1]%32));
            }
        }
    }
    else
    {
    //    memcpy(matrixH,matrixA,offset*nb_pcb*sizeof(int32_t));
        for(int32_t i=0;i< nb_pcb;i++)
            memcpy(matrixH+i*offset,matrixA+column_arrangement[i]*offset,offset*sizeof(int32_t));

        //swap columns
        int32_t tmp=0;
        for(int32_t i=0;i<loop;i++)
        {
            for (int32_t j=0;j<nb_pcb;j++)
            {
                tmp ^= (-((matrixH[swap_col[2*i]/32+j*offset] >> (31-swap_col[2*i]%32)) & 1) ^ tmp) & (1 << 0);
                matrixH[swap_col[2*i]/32+j*offset]   ^= (-((matrixH[swap_col[2*i+1]/32+j*offset] >> (31-swap_col[2*i+1]%32)) & 1) ^ matrixH[swap_col[2*i]/32+j*offset]) & (1 << (31-swap_col[2*i]%32));
                matrixH[swap_col[2*i+1]/32+offset*j] ^= (-((tmp >> 0) & 1) ^ matrixH[swap_col[2*i+1]/32+offset*j]) & (1 << (31-swap_col[2*i+1]%32));
            }
        }
        memcpy(matrixA,matrixH,offset*nb_pcb*sizeof(int32_t));
    }

    free(column_arrangement);
    free(processed_column);
    free(zero_lines_nb);
    free(swap_col);
    free(matrixH);
    return 0;
}


std::vector<int32_t> createMetadataMatrixA(int32_t wc, int32_t capacity)
{
  int32_t nb_pcb=capacity/2;
  int32_t offset=ceil(capacity/(float_t)32);

  std::vector<int32_t> matrixA(offset*nb_pcb*4);
  std::vector<int32_t>permutation(capacity*4);
  for (int32_t i = 0; i<capacity; i++) {
    permutation[i]=i;
  }
  setSeed(LPDC_METADATA_SEED);
  int32_t nb_once=capacity*nb_pcb/(float_t)wc+3;
  nb_once=nb_once/nb_pcb;
  for (int32_t i = 0; i<nb_pcb; i++) {
    for (int32_t j = 0; j<nb_once; j++) {
      int32_t pos = (int32_t)((float_t) lcg64_temper()/(float_t)UINT32_MAX * (capacity-j));
      matrixA[i*offset+permutation[pos]/32] = 1<<(31-permutation[pos]%32);
      int32_t tmp = permutation[capacity-1-j];
      permutation[capacity-1-j]= permutation[pos];
      permutation[pos]=tmp;
    }
  }
  return  matrixA;
}

std::vector<int32_t> createGeneratorMatrix(std::vector<int32_t> matrixA,int32_t Pn)
{
  int32_t capacity = matrixA.size();
  int32_t effwidth = ceil(Pn/(float_t)32)*32;
  int32_t offset = ceil(Pn/(float_t)32);
  int32_t offset_cap = ceil(capacity/(float_t)32);

  std::vector<int32_t> G(offset*capacity*4);

  for (int32_t i = 0; i<Pn; i++) {
    G[(capacity-Pn+i)*offset+i/32] = 1<<(31-i%32);
  }
  
    int32_t matrix_index=capacity-Pn;
    int32_t loop=0;

    for (int32_t i=0; i<(capacity-Pn)*effwidth; i++)
    {
        if(matrix_index >= capacity)
        {
            loop++;
            matrix_index=capacity-Pn;
        }
        if(i % effwidth < Pn)
        {
            G[i/32] ^= (-((matrixA[matrix_index/32+offset_cap*loop] >> (31-matrix_index%32)) & 1) ^ G[i/32]) & (1 << (31-i%32));
            matrix_index++;
        }
    }
    return G;
}

