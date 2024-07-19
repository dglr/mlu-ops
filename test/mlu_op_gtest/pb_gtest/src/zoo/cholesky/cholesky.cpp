/*************************************************************************
 * Copyright (C) [2022] by Cambricon, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *************************************************************************/
#include <vector>
#include "cholesky.h"

namespace mluoptest {


void CholeskyExecutor::paramCheck() {
  if (parser_->getInputNum() != 1) {
    LOG(ERROR) << "cholesky input number is wrong. ";
  }
  if (parser_->getOutputNum() != 1) {
    LOG(ERROR) << "cholesky output number is wrong. ";
  }
  flag_quant_mode_ = NO_QUANT;
  
  
  
}

void set_matrix_zero(float*A, bool upper, bool trans_, int n_, int ldda_, mluOpDataType_t type_)
{
    if(trans_)
    {
        for (long int i = 0; i < n_; i++)
        {
            for (long int j = 0; j < ldda_; j++)
            {
                if(upper)
                {
                    if(i >= j) 
                    {
                        if(i == j && type_ == MLUOP_DTYPE_COMPLEX_FLOAT)
                        {
                            A[(j + i * ldda_)*2+1] = 0.0;
                        } 
                        else
                        {
                            if(type_ == MLUOP_DTYPE_FLOAT)
                                A[j + i * ldda_] = 0.0;
                            else
                            {
                                A[(j + i * ldda_)*2] = 0.0;
                                A[(j + i * ldda_)*2+1] = 0.0;
                            }
                        }
                        
                    }    
                }
                else 
                {
                    if(i <= j)
                    {
                        if(i == j)
                        {
                            if(type_ == MLUOP_DTYPE_COMPLEX_FLOAT)
                            {
                                A[(j + i * ldda_)*2+1] = 0.0;
                            }
                        }
                        else
                        {
                            if(type_ == MLUOP_DTYPE_FLOAT)
                                A[j + i * ldda_] = 0.0;
                            else
                            {
                                A[(j + i * ldda_)*2] = 0.0;
                                A[(j + i * ldda_)*2+1] = 0.0;
                            }
                        }
                        
                    }
                }
            }
        }
    }
    else
    {
        for (int i = 0; i < n_; i++)
        {
            for (int j = 0; j < ldda_; j++)
            {
              if((i > j && ~upper)||(i < j && upper))
              {
                if(type_ == MLUOP_DTYPE_FLOAT)
                    A[j + i * ldda_] = 0.0;
                else
                {
                    A[(j + i * ldda_)*2] = 0.0;
                    A[(j + i * ldda_)*2+1] = 0.0;
                }
              }
            }
        }
    }
    
}

void trans_mul(float*A, float*C, int lda,bool upper_, bool trans_, int n_, int ldda_, mluOpDataType_t type_, bool diag_add)
{
    if(trans_)
    {
        for(long int i = 0; i <lda; i++)
        {
            for(long int j = 0;j < n_; j++)
            {
                if(type_ == MLUOP_DTYPE_FLOAT)
                {
                    A[i+j*lda] = 0.0;
                    if(j == i && diag_add)
                        A[j+i*lda] = 1.0;
                    
                }
                    
                else if(type_ == MLUOP_DTYPE_COMPLEX_FLOAT && ((upper_==false && j >= i) || (upper_==true && j >= i)))
                {
                    A[j*lda*2+i*2] = 0.0;
                    A[j*lda*2+i*2+1] = 0.0;
                    if(j == i&& diag_add)
                        A[j*lda*2+i*2] = 1.0;
                }
                for(long int k = 0; k <=i; k++)
                {
                    if(upper_==false)
                    {
                        if(j < i)
                            continue;
                        else
                        {
                            if(type_ == MLUOP_DTYPE_FLOAT)
                                A[i+j*lda] += (C[k+i*lda]*C[k+j*lda]);
                            else
                            {
                                A[(i+j*lda)*2] += (C[(k+i*lda)*2]*C[(k+j*lda)*2]+C[(k+i*lda)*2+1]*C[(k+j*lda)*2+1]);
                                A[(i+j*lda)*2+1] += (C[(k+i*lda)*2]*C[(k+j*lda)*2+1]-C[(k+i*lda)*2+1]*C[(k+j*lda)*2]);
                            }
                        }
                        if(type_ != MLUOP_DTYPE_FLOAT && j != i)
                        {
                            A[(j+i*lda)*2] = A[(i+j*lda)*2];
                            A[(j+i*lda)*2+1] = -A[(i+j*lda)*2+1];
                        }
                    }
                    else
                    {
                        if(type_ == MLUOP_DTYPE_FLOAT)
                        {
                            if(j > i)
                                continue;
                            else
                            {
                                A[i+j*lda] += (C[k*lda+i]*C[k*lda+j]);
                            }
                        }
                        else
                        {
                            if(j < i)
                                continue;
                            else
                            {
                                A[(i+j*lda)*2] += (C[(k+i*lda)*2]*C[(k+j*lda)*2]+C[(k+i*lda)*2+1]*C[(k+j*lda)*2+1]);
                                A[(i+j*lda)*2+1] += (C[(k+i*lda)*2]*C[(k+j*lda)*2+1]-C[(k+i*lda)*2+1]*C[(k+j*lda)*2]);
                            }
                            if(type_ != MLUOP_DTYPE_FLOAT && j != i)
                            {
                                A[(j+i*lda)*2] = A[(i+j*lda)*2];
                                A[(j+i*lda)*2+1] = -A[(i+j*lda)*2+1];
                            }
                        }
                        

                        
                    }
                }
                if(type_ != MLUOP_DTYPE_FLOAT &&((upper_==false && j > i) || (upper_==true && j > i)))
                {
                    A[(j+i*lda)*2] = A[(i+j*lda)*2];
                    A[(j+i*lda)*2+1] = -A[(i+j*lda)*2+1];
                }
            }
        }
    }
    else
    {
        for(int i = 0; i <lda; i++)
        {
            for(int j = 0;j < n_; j++)
            {
                if(type_ == MLUOP_DTYPE_FLOAT)
                    A[j+i*lda] = 0.0;
                else
                {
                    A[(i+j*lda)*2] = 0.0;
                    A[(i+j*lda)*2+1] = 0.0;
                }
                for(int k = 0; k <=i; k++)
                {
                    if(j < i)
                        continue;
                    if(type_ == MLUOP_DTYPE_FLOAT)
                        A[j+i*lda] += (C[j+k*lda]*C[i+k*lda]);
                    else
                    {
                        A[(j+i*lda)*2] += (C[(j+k*lda)*2]*C[(i+k*lda)*2]);
                        A[(j+i*lda)*2+1] += (C[(j+k*lda)*2+1]*C[(i+k*lda)*2+1]);
                    }
                }
            }
        }
    }    
}

void fill_zero(float*A, bool upper_, int batch_, int n_, int ldda_, mluOpDataType_t type_,bool if_conj)
{
    int stride = n_ * ldda_;
    if(type_ == MLUOP_DTYPE_FLOAT)
    {
    }
    else
    {
        stride *= 2;
    }
    for(long int i = 0; i < batch_;i++)
    {
        for(long int j = 0; j < n_; j++)
        {
            for(long int h = 0; h < ldda_; h++)
            {
                if(j==h)
                {
                    continue;
                }
                else if(j<h)
                {
                    if(upper_)
                        continue;
                }
                else
                {
                    if(upper_ == false)
                        continue;
                }
                if(type_ == MLUOP_DTYPE_FLOAT)
                {
                    A[i*stride+j*ldda_+h] = A[i*stride+h*ldda_+j];
                }
                else
                {
                    A[i*stride+j*ldda_*2+h*2] = A[i*stride+h*ldda_*2+j*2];
                    if(if_conj)
                        A[i*stride+j*ldda_*2+h*2+1] = -A[i*stride+h*ldda_*2+j*2+1];
                    else
                        A[i*stride+j*ldda_*2+h*2+1] = A[i*stride+h*ldda_*2+j*2+1];
                }
            }
        }
    }   

}

void set_diag_imag_one(float*A,int batch_,int n_,int ldda_)
{
    long int stride = n_ * ldda_ * 2;
    for(long int i = 0; i < batch_;i++)
    {
        for(long int j = 0; j < n_; j++)
        {
            A[i*stride+(j*ldda_+j)*2+1] = 1.0;
        }
    }

}

void print_matrix(int batch, float*A, int lda, bool trans_, int n_, int ldda_, mluOpDataType_t type_)
{
    for(int x = 0; x < batch; x++)
    {
        printf("batch:%d\n",x);
        if(trans_)
        {
            for(int i = 0; i <n_; i++)
            {
                for(int j = 0; j <lda; j++)
                {
                    if(type_ == MLUOP_DTYPE_FLOAT)
                        printf("%7.3f ",A[j+i*lda]);
                    else
                    {
                        printf("%7.3f",A[(j+i*lda)*2]);
                        printf(",");
                        printf("%7.3f ",A[(j+i*lda)*2+1]);
                    }
                }
                printf("\n");
            }
        }
        else
        {
            for(int i = 0; i <lda; i++)
            {
                for(int j = 0; j <n_; j++)
                {
                    if(type_ == MLUOP_DTYPE_FLOAT)
                        printf("%7.3f ",A[j+i*lda]);
                    else
                    {
                        printf("%7.3f",A[(j+i*lda)*2]);
                        printf(",");
                        printf("%7.3f ",A[(j+i*lda)*2+1]);
                    }
                }
                printf("\n");
            }
        }
        A += n_ * lda;
        if(type_ == MLUOP_DTYPE_COMPLEX_FLOAT)
            A += n_ * lda;
    }
    
}

void CholeskyExecutor::prepareComputeParam()
{

  printf("start prepare compute parameter.\n");
  int long_int_size = sizeof(long int);
  int int_size = sizeof(int);
  printf("long int size:%d, int size:%d\n",long_int_size,int_size);
  auto input_desc_ = (tensor_desc_[0].tensor);
  auto output_desc_ = (tensor_desc_[1].tensor);
  auto dev_a = (float*)(data_vector_[0].host_ptr);
  auto dev_c = (float*)(data_vector_[1].host_ptr);
  auto dev_d = (float*)(data_vector_[0].device_ptr);
  auto input_tensor = parser_->getProtoNode()->input(0);
  auto input_shape = input_tensor.shape();
  auto base_line_out = cpu_fp32_output_[0];
  upper_ = parser_->getProtoNode()->cholesky_param().upper();
  int dim_size = input_shape.dims_size();
  type_ = input_desc_->dtype;
  type_size_ = type_ == MLUOP_DTYPE_FLOAT ? 4 : 8;
  if(dim_size ==2)
  {
    n_ = input_shape.dims(0); 
    int dim = input_desc_->dim;
    stride_ = (input_desc_->strides)[dim-1];
    ldda_ = input_desc_->dims[1];
    printf("n:%d,lda:%d,stride:%d,upper:%d,trans:%d\n",n_,ldda_,stride_,upper_,trans_);
    int size = input_desc_->dims[0];
    printf("size:%d, dim:%d, \n",size,dim);
    printf("strides:\n");
    for(int i = 0; i < dim; i++)
    {
      printf("%ld ",(input_desc_->strides)[i]);
    }
    printf("\n");
    printf("data vector length : %ld\n",data_vector_.size());
  }
  else if(dim_size == 3)
  {
    batch_size_ = input_shape.dims(0);
    n_ = input_shape.dims(1); 
    int dim = input_desc_->dim;
    stride_ = (input_desc_->strides)[dim-1];
    ldda_ = input_desc_->dims[2];
    printf("batch_size:%ld,n:%d,lda:%d,stride:%d,upper:%d,trans:%d\n",batch_size_,n_,ldda_,stride_,upper_,trans_);

    int size = input_desc_->dims[1];

    printf("size:%d, dim:%d, \n",size,dim);
    printf("strides:\n");
    for(int i = 0; i < dim; i++)
    {
      printf("%ld ",(input_desc_->strides)[i]);
    }
    printf("\n");
    printf("data vector length : %ld\n",data_vector_.size());
  }
  
  if(batch_size_ > 16 && n_ > 2000)
  {
    std::memcpy(dev_c,dev_a,16*type_size_*n_*ldda_);
    std::memcpy(dev_c+16*type_size_/4*n_*ldda_,dev_a+16*type_size_/4*n_*ldda_,(batch_size_-16)*type_size_*n_*ldda_);
  }
  else
  {
    std::memcpy(dev_c,dev_a,batch_size_*type_size_*n_*ldda_);
  }
  if(parser_->device() == CPU)
  {
    for(long int i = 0; i < batch_size_;i++)
    {
      if(type_ == MLUOP_DTYPE_FLOAT)
          set_matrix_zero(dev_c+i*n_*ldda_,false,trans_,n_,ldda_,type_);
      else
          set_matrix_zero(dev_c+i*n_*ldda_*2,false,trans_,n_,ldda_,type_);
    }
    for(long int i = 0; i < batch_size_;i++)
    {
      if(type_ == MLUOP_DTYPE_FLOAT)
      {
          trans_mul(dev_a+i*n_*ldda_,dev_c+i*n_*ldda_,ldda_,false,trans_,n_,ldda_,type_,true);
          fill_zero(dev_a,false,batch_size_,n_,ldda_,type_,false);
      }
      else
      {
          trans_mul(dev_a+i*n_*ldda_*2,dev_c+i*n_*ldda_*2,ldda_,false,trans_,n_,ldda_,type_,true);
          fill_zero(dev_a,false,batch_size_,n_,ldda_,type_,true);
      }
    }
  }
  
  
    

  if(batch_size_>16)
  {
    GTEST_CHECK(CNRT_RET_SUCCESS ==
                  cnrtMemcpy(dev_d, dev_a, type_size_*n_*ldda_*16, CNRT_MEM_TRANS_DIR_HOST2DEV));
    GTEST_CHECK(CNRT_RET_SUCCESS ==
                  cnrtMemcpy(dev_d+16*type_size_/4*n_*ldda_, dev_a+16*type_size_/4*n_*ldda_, type_size_*n_*ldda_*(batch_size_-16), CNRT_MEM_TRANS_DIR_HOST2DEV));
  }
  else
  {
    GTEST_CHECK(CNRT_RET_SUCCESS ==
                  cnrtMemcpy(dev_d, dev_a, type_size_*n_*ldda_*batch_size_, CNRT_MEM_TRANS_DIR_HOST2DEV));
  }
  
  if(parser_->device() == CPU)
  {
    float* cpu_a = cpu_fp32_input_[0];
    if(batch_size_ > 16 && n_ > 2000)
    {
      std::memcpy(cpu_a,dev_a,16*type_size_*n_*ldda_);
      std::memcpy(cpu_a+16*type_size_/4*n_*ldda_,dev_a+16*type_size_/4*n_*ldda_,(batch_size_-16)*type_size_*n_*ldda_);
    }
    else
    {
      std::memcpy(cpu_a,dev_a,batch_size_*type_size_*n_*ldda_);
    }
  }
  
  
  printf("end prepare compute.\n");

}

void CholeskyExecutor::compute() {
  
  
  VLOG(4) <<"  CholeskyExecutor compute ";
  auto input_desc_ = tensor_desc_[0].tensor;
  auto output_desc_ = tensor_desc_[1].tensor;
  auto h_input = (float*)(data_vector_[0].host_ptr);
  auto h_output = (float*)(data_vector_[1].host_ptr);
  auto d_intput = (float*)(data_vector_[0].device_ptr);
  auto d_output = (float*)(data_vector_[1].device_ptr);
  if(batch_size_>16)
  {
    std::memcpy(h_input,h_output,type_size_*n_*ldda_*16);
    std::memcpy(h_input+type_size_/4*n_*ldda_*16,h_output+type_size_/4*n_*ldda_*16,type_size_*n_*ldda_*(batch_size_-16));
  }
  else
  {
    std::memcpy(h_input,h_output,type_size_*n_*ldda_*batch_size_);
  }
  if(batch_size_>16)
  {
    GTEST_CHECK(CNRT_RET_SUCCESS ==
                  cnrtMemcpy(h_output, d_intput, type_size_*n_*ldda_*16, CNRT_MEM_TRANS_DIR_DEV2HOST));
    GTEST_CHECK(CNRT_RET_SUCCESS ==
                  cnrtMemcpy(h_output+16*type_size_/4*n_*ldda_, d_intput+16*type_size_/4*n_*ldda_, type_size_*n_*ldda_*(batch_size_-16), CNRT_MEM_TRANS_DIR_DEV2HOST));
  }
  else
  {
    GTEST_CHECK(CNRT_RET_SUCCESS ==
                  cnrtMemcpy(h_output, d_intput, type_size_*n_*ldda_*batch_size_, CNRT_MEM_TRANS_DIR_DEV2HOST));
  }
  
  interface_timer_.start();
  float* workspace = nullptr;
  size_t size = 0;
  mluOpGetCholeskyWorkspace(input_desc_,&size,&workspace);

MLUOP_CHECK(mluOpCholesky(handle_,input_desc_,d_intput, output_desc_, d_output, upper_,workspace));

  interface_timer_.stop();
  if(batch_size_>16)
  {
    GTEST_CHECK(CNRT_RET_SUCCESS ==
                  cnrtMemcpy(h_output, d_output, 16*type_size_*n_*ldda_, CNRT_MEM_TRANS_DIR_DEV2HOST));
    GTEST_CHECK(CNRT_RET_SUCCESS ==
                  cnrtMemcpy(h_output+16*type_size_/4*n_*ldda_, d_output+16*type_size_/4*n_*ldda_, (batch_size_-16)*type_size_*n_*ldda_, CNRT_MEM_TRANS_DIR_DEV2HOST));
  }
  else
  {
    GTEST_CHECK(CNRT_RET_SUCCESS ==
                  cnrtMemcpy(h_output, d_output, batch_size_*type_size_*n_*ldda_, CNRT_MEM_TRANS_DIR_DEV2HOST));
  }

  if(parser_->device() != CPU )
  {
    if(result_mul)
    {
        for(int i = 0; i < batch_size_;i++)
        {
          if(type_ == MLUOP_DTYPE_FLOAT)
              trans_mul(h_input+i*n_*ldda_,h_output+i*n_*ldda_,ldda_,upper_,trans_,n_,ldda_,type_,false);
          else
              trans_mul(h_input+i*n_*ldda_*2,h_output+i*n_*ldda_*2,ldda_,upper_,trans_,n_,ldda_,type_,false);
        }
    }
    else
    {
        
        fill_zero(h_output,upper_,batch_size_,n_,ldda_,type_,false); 
    }
    if(type_ != MLUOP_DTYPE_FLOAT)
    {
        set_diag_imag_one(h_output,batch_size_,n_,ldda_);
    }
    if(batch_size_>16)
    {
      GTEST_CHECK(CNRT_RET_SUCCESS ==
                    cnrtMemcpy(d_output, h_output, 16*type_size_*n_*ldda_, CNRT_MEM_TRANS_DIR_HOST2DEV));
      GTEST_CHECK(CNRT_RET_SUCCESS ==
                    cnrtMemcpy(d_output+16*type_size_/4*n_*ldda_, h_output+16*type_size_/4*n_*ldda_, (batch_size_-16)*type_size_*n_*ldda_, CNRT_MEM_TRANS_DIR_HOST2DEV));
    }
    else
    {
      GTEST_CHECK(CNRT_RET_SUCCESS ==
                    cnrtMemcpy(d_output, h_output, batch_size_*type_size_*n_*ldda_, CNRT_MEM_TRANS_DIR_HOST2DEV));
    }
    
  }

  return;
}

void cpu_compute(float* cpu_c, int n_, int ldda_, bool upper_, bool trans_, mluOpDataType_t type_)
{
    if(trans_)
    {
        for(long int i = 0; i < n_; i++)
        {
            float dia;
            if(type_ == MLUOP_DTYPE_FLOAT)
            {
                dia = cpu_c[i+i*ldda_];
            }
            else
            {
                dia = cpu_c[(i+i*ldda_)*2];
            }
            float dia_root = sqrt(dia);
            
            if(type_ == MLUOP_DTYPE_FLOAT)
            {
                cpu_c[i+i*ldda_] = sqrt(dia);
            }
            else
            {
                cpu_c[(i+i*ldda_)*2] = sqrt(dia);
            }
            if(upper_==false)
            {
                if(type_ == MLUOP_DTYPE_FLOAT)
                {
                    for(long int j = i+1;j<n_;j++)
                    {
                        cpu_c[i+j*ldda_] = cpu_c[i+j*ldda_]/dia_root;   
                    }
                    for(long int j = i+1;j < n_;j++)
                    {
                        for(long int k = j;k <n_;k++)
                        {
                            cpu_c[j + k * ldda_] -= (cpu_c[i + k*ldda_] * cpu_c[i + j * ldda_]); 
                        }
                    }
                }
                else
                {
                    for(long int j = 0; j < i; j++)
                    {
                        cpu_c[(i+j*ldda_)*2] = 0;   
                        cpu_c[(i+j*ldda_)*2+1] = 0;
                    }
                    for(long int j = i+1;j<n_;j++)
                    {
                        cpu_c[(i+j*ldda_)*2] = cpu_c[(i+j*ldda_)*2]/dia_root;   
                        cpu_c[(i+j*ldda_)*2+1] = cpu_c[(i+j*ldda_)*2+1]/dia_root; 
                    }
                    for(long int j = i+1;j < n_;j++)
                    {
                        for(long int k = j;k <n_;k++)
                        {
                            cpu_c[(j + k * ldda_)*2] -= (cpu_c[(i + k*ldda_)*2] * cpu_c[(i + j * ldda_)*2]+cpu_c[(i + k*ldda_)*2+1] * cpu_c[(i + j * ldda_)*2+1]); 
                            cpu_c[(j + k * ldda_)*2+1] -= (cpu_c[(i + k*ldda_)*2+1] * cpu_c[(i + j * ldda_)*2]-cpu_c[(i + k*ldda_)*2] * cpu_c[(i + j * ldda_)*2+1]); 
                        }
                    }
                }
                
            }
            else
            {
                if(type_ == MLUOP_DTYPE_FLOAT)
                {
                    for(long int j = i+1;j<n_;j++)
                    {
                        cpu_c[j+i*ldda_] = cpu_c[j+i*ldda_]/dia_root;   
                    }
                    for(long int j = i+1;j < n_;j++)
                    {
                        for(long int k = j;k <n_;k++)
                        {
                            cpu_c[k + j * ldda_] -= (cpu_c[k + i*ldda_] * cpu_c[j + i * ldda_]); 
                        }
                    }
                }
                else
                {

                    for(long int j = i+1;j<n_;j++)
                    {
                        cpu_c[(j+i*ldda_)*2] = cpu_c[(j+i*ldda_)*2]/dia_root;   
                        cpu_c[(j+i*ldda_)*2+1] = cpu_c[(j+i*ldda_)*2+1]/dia_root;  
                    }
                    for(long int j = i+1;j < n_;j++)
                    {
                        for(long int k = j;k <n_;k++)
                        {
                            cpu_c[(k + j * ldda_)*2] -= (cpu_c[(k + i*ldda_)*2] * cpu_c[(j + i * ldda_)*2]+cpu_c[(k + i*ldda_)*2+1] * cpu_c[(j + i * ldda_)*2+1]); 
                            cpu_c[(k + j * ldda_)*2+1] -= (cpu_c[(k + i*ldda_)*2+1] * cpu_c[(j + i * ldda_)*2]-cpu_c[(k + i*ldda_)*2] * cpu_c[(j + i * ldda_)*2+1]); 
                        }
                    }
                }
                
            }
            
        }
    }
    else
    {
        for(int i = 0; i < ldda_; i++)
        {
            float dia = cpu_c[i+i*ldda_];
            float dia_root = sqrt(dia);
            cpu_c[i+i*ldda_] = sqrt(dia);
            for(int j = i+1;j<ldda_;j++)
            {
                cpu_c[j+i*ldda_] = cpu_c[j+i*ldda_]/dia_root;   
            }
            for(int j = i+1;j < ldda_;j++)
            {
                for(int k = j;k <ldda_;k++)
                {
                    cpu_c[k + j * ldda_] -= (cpu_c[k + i*ldda_] * cpu_c[j + i * ldda_]); 
                }
            }
        }
    }
}

void CholeskyExecutor::cpuCompute() {
   auto dev_c = (float*)(data_vector_[0].host_ptr);
   float* cpu_a = cpu_fp32_input_[0];
   float* cpu_c = cpu_fp32_output_[0];
  
    if(batch_size_>16)
    {
      std::memcpy(cpu_c,cpu_a,type_size_*n_*ldda_*16);
      std::memcpy(cpu_c+type_size_/4*n_*ldda_*16,cpu_a+type_size_/4*n_*ldda_*16,type_size_*n_*ldda_*(batch_size_-16));
    }
    else
    {
      std::memcpy(cpu_c,cpu_a,type_size_*n_*ldda_*batch_size_);
    }

    auto h_output = (float*)(data_vector_[1].host_ptr);
    auto h_input = (float*)(data_vector_[0].host_ptr);

    printf("cpu before cholesky result:\n");

    if(result_mul)
    {
      for(int i = 0; i < batch_size_;i++)
      {
        if(type_ == MLUOP_DTYPE_FLOAT)
            trans_mul(h_input+i*n_*ldda_,h_output+i*n_*ldda_,ldda_,upper_,trans_,n_,ldda_,type_,false);
        else
            trans_mul(h_input+i*n_*ldda_*2,h_output+i*n_*ldda_*2,ldda_,upper_,trans_,n_,ldda_,type_,false);
      }
      if(batch_size_>16)
      {
        std::memcpy(h_output,h_input,type_size_*n_*ldda_*16);
        std::memcpy(h_output+type_size_/4*n_*ldda_*16,h_input+type_size_/4*n_*ldda_*16,type_size_*n_*ldda_*(batch_size_-16));
      }
      else
      {
        std::memcpy(h_output,h_input,type_size_*n_*ldda_*batch_size_);
      }
    }
    else
    {
        for(long int i = 0; i < batch_size_;i++)
        {
            cpu_compute(cpu_c+i*n_*ldda_*type_size_/4, n_, ldda_, upper_, trans_, type_);
        }
        fill_zero(cpu_c,upper_,batch_size_,n_,ldda_,type_,false);
        fill_zero(h_output,upper_,batch_size_,n_,ldda_,type_,false);
    }


	return;
}


int64_t CholeskyExecutor::getTheoryOps() {
  int64_t theory_ops = batch_size_*n_*n_*n_/2;
  VLOG(4) << "getTheoryOps: " << theory_ops << " ops";
  return theory_ops;
}
}  // namespace mluoptest
