
// Test target codegen - host bc file has to be created first.
// RUN: %clang_cc1 -verify -fopenmp -x c++ -triple powerpc64le-unknown-unknown -fomptargets=nvptx64-nvidia-cuda -emit-llvm-bc %s -o %t-ppc-host.bc
// RUN: %clang_cc1 -verify -fopenmp -x c++ -triple nvptx64-unknown-unknown -fomptargets=nvptx64-nvidia-cuda -emit-llvm %s -fopenmp-is-device -fomp-host-ir-file-path %t-ppc-host.bc -o - | FileCheck %s --check-prefix TCHECK --check-prefix TCHECK-64
// RUN: %clang_cc1 -verify -fopenmp -x c++ -triple i386-unknown-unknown -fomptargets=nvptx-nvidia-cuda -emit-llvm-bc %s -o %t-x86-host.bc
// RUN: %clang_cc1 -verify -fopenmp -x c++ -triple nvptx-unknown-unknown -fomptargets=nvptx-nvidia-cuda -emit-llvm %s -fopenmp-is-device -fomp-host-ir-file-path %t-x86-host.bc -o - | FileCheck %s --check-prefix TCHECK --check-prefix TCHECK-32
// expected-no-diagnostics
#ifndef HEADER
#define HEADER

template<typename tx, typename ty>
struct TT{
  tx X;
  ty Y;
};

// TCHECK:  [[TT:%.+]] = type { i64, i8 }
// TCHECK:  [[S1:%.+]] = type { double }

int foo(int n, double *ptr) {
  int a = 0;
  short aa = 0;
  float b[10];
  double c[5][10];
  TT<long long, char> d;
  
  #pragma omp target firstprivate(a)
  {
  }
  
  // TCHECK:  define void @__omp_offloading_{{.+}}(i{{[0-9]+}} [[A_IN:%.+]])
  // TCHECK:  [[A_ADDR:%.+]] = alloca i{{[0-9]+}},
  // TCHECK:  store i{{[0-9]+}} [[A_IN]], i{{[0-9]+}}* [[A_ADDR]],
  // TCHECK-64:  [[CONV:%.+]] = bitcast i{{[0-9]+}}* [[A_ADDR]] to i{{[0-9]+}}*
  // TCHECK:  ret void  

#pragma omp target firstprivate(aa,b,c,d)
  {
    aa += 1;
    b[2] = 1.0;
    c[1][2] = 1.0;
    d.X = 1;
    d.Y = 1;    
  }
  
  // the input parameters to the offloading function are already private variables to the target region
  // TCHECK:  define void @__omp_offloading_{{.+}}(i{{[0-9]+}} [[A2_IN:%.+]], [10 x float]* {{.+}} [[B_IN:%.+]], [5 x [10 x double]]* {{.+}} [[C_IN:%.+]], [[TT]]* {{.+}} [[D_IN:%.+]])
  // TCHECK:  [[A2_ADDR:%.+]] = alloca i{{[0-9]+}},
  // TCHECK:  [[B_ADDR:%.+]] = alloca [10 x float]*,
  // TCHECK:  [[C_ADDR:%.+]] = alloca [5 x [10 x double]]*,
  // TCHECK:  [[D_ADDR:%.+]] = alloca [[TT]]*,
  // TCHECK:  store i{{[0-9]+}} [[A2_IN]], i{{[0-9]+}}* [[A2_ADDR]],
  // TCHECK:  store [10 x float]* [[B_IN]], [10 x float]** [[B_ADDR]],
  // TCHECK:  store [5 x [10 x double]]* [[C_IN]], [5 x [10 x double]]** [[C_ADDR]],
  // TCHECK:  store [[TT]]* [[D_IN]], [[TT]]** [[D_ADDR]],
  // TCHECK:  [[CONV_A2ADDR:%.+]] = bitcast i{{[0-9]+}}* [[A2_ADDR]] to i{{[0-9]+}}*
  // TCHECK:  [[B_ADDR_REF:%.+]] = load [10 x float]*, [10 x float]** [[B_ADDR]],
  // TCHECK:  [[C_ADDR_REF:%.+]] = load [5 x [10 x double]]*, [5 x [10 x double]]** [[C_ADDR]],
  // TCHECK:  [[D_ADDR_REF:%.+]] = load [[TT]]*, [[TT]]** [[D_ADDR]],

  // aa += 1 (only check load and store are from and to right variable)
  // TCHECK:  {{.+}} = load i{{[0-9]+}}, i{{[0-9]+}}* [[CONV_A2ADDR]],
  // TCHECK:  store i{{[0-9]+}} {{.+}}, i{{[0-9]+}}* [[CONV_A2ADDR]],
  
  //  b[2] = 1.0
  // TCHECK: [[B_GEP:%.+]] = getelementptr inbounds [10 x float], [10 x float]* [[B_ADDR_REF]],
  // TCHECK: store float {{.+}}, float* [[B_GEP]],

  // c[1][2] = 1.0
  // TCHECK:  [[B_GEP1:%.+]] = getelementptr inbounds [5 x [10 x double]], [5 x [10 x double]]* [[C_ADDR_REF]],
  // TCHECK:  [[B_GEP2:%.+]] = getelementptr inbounds [10 x double], [10 x double]* [[B_GEP1]],
  // TCHECK:  store double {{.+}}, double* [[B_GEP2]],
  
  // d.X = 1; d.Y = 1
  // TCHECK:  [[X:%.+]] = getelementptr inbounds [[TT]], [[TT]]* [[D_ADDR_REF]],
  // TCHECK:  store i{{[0-9]+}} {{.+}}, i{{[0-9]+}}* [[X]]
  // TCHECK:  [[Y:%.+]] = getelementptr inbounds [[TT]], [[TT]]* [[D_ADDR_REF]],
  // TCHECK:  store i{{[0-9]+}} {{.+}}, i{{[0-9]+}}* [[Y]]

#pragma omp target firstprivate(ptr)
  {
    ptr[0]++;
  }

  // TCHECK:  define void @__omp_offloading_{{.+}}(double* [[PTR_IN:%.+]])
  // TCHECK:  [[PTR_ADDR:%.+]] = alloca double*,
  // TCHECK:  store double* [[PTR_IN]], double** [[PTR_ADDR]],
  // TCHECK:  [[PTR_IN_REF:%.+]] = load double*, double** [[PTR_ADDR]],
  // TCHECK:  [[PTR_GEP:%.+]] = getelementptr inbounds double, double* [[PTR_IN_REF]],
  // TCHECK:  store double {{.+}}, double* [[PTR_GEP]],

  return a;
}


template<typename tx>
tx ftemplate(int n) {
  tx a = 0;
  tx b[10];

#pragma omp target firstprivate(a,b)
  {
    a += 1;
    b[2] += 1;
  }

  return a;
}

static
int fstatic(int n) {
  int a = 0;
  char aaa = 0;
  int b[10];

#pragma omp target firstprivate(a,aaa,b)
  {
    a += 1;
    aaa += 1;
    b[2] += 1;
  }

  return a;
}

// TCHECK: define void @__omp_offloading_{{.+}}(i{{[0-9]+}} [[A_IN:%.+]], i{{[0-9]+}} [[A3_IN:%.+]], [10 x i{{[0-9]+}}]*{{.+}} [[B_IN:%.+]])
// TCHECK:  [[A_ADDR:%.+]] = alloca i{{[0-9]+}},
// TCHECK:  [[A3_ADDR:%.+]] = alloca i{{[0-9]+}},
// TCHECK:  [[B_ADDR:%.+]] = alloca [10 x i{{[0-9]+}}]*,
// TCHECK:  store i{{[0-9]+}} [[A_IN]], i{{[0-9]+}}* [[A_ADDR]],
// TCHECK:  store i{{[0-9]+}} [[A3_IN]], i{{[0-9]+}}* [[A3_ADDR]],
// TCHECK:  store [10 x i{{[0-9]+}}]* [[B_IN]], [10 x i{{[0-9]+}}]** [[B_ADDR]],
// TCHECK-64:  [[A_CONV:%.+]] = bitcast i{{[0-9]+}}* [[A_ADDR]] to i{{[0-9]+}}*
// TCHECK:  [[A3_CONV:%.+]] = bitcast i{{[0-9]+}}* [[A3_ADDR]] to i8*
// TCHECK:  [[B_ADDR_REF:%.+]] = load [10 x i{{[0-9]+}}]*, [10 x i{{[0-9]+}}]** [[B_ADDR]],

// a += 1
// TCHECK-64:  {{.+}} = load i{{[0-9]+}}, i{{[0-9]+}}* [[A_CONV]],
// TCHECK-32:  {{.+}} = load i{{[0-9]+}}, i{{[0-9]+}}* [[A_ADDR]],
// TCHECK-64:  store i{{[0-9]+}} {{.+}}, i{{[0-9]+}}* [[A_CONV]],
// TCHECK-32:  store i{{[0-9]+}} {{.+}}, i{{[0-9]+}}* [[A_ADDR]],

// aaa += 1
// TCHECK:  {{.+}} = load i{{[0-9]+}}, i{{[0-9]+}}* [[A3_CONV]],
// TCHECK:  store i{{[0-9]+}} {{.+}}, i{{[0-9]+}}* [[A3_CONV]],

// b[2] += 1
// TCHECK:  [[B_GEP:%.+]] = getelementptr inbounds [10 x i{{[0-9]+}}], [10 x i{{[0-9]+}}]* [[B_ADDR_REF]],
// TCHECK:  store i{{[0-9]+}} {{.+}}, i{{[0-9]+}}* [[B_GEP]],  

// TCHECK:  ret void

struct S1 {
  double a;

  int r1(int n){
    int b = n+1;

#pragma omp target firstprivate(b)
    {
      this->a = (double)b + 1.5;
    }

    return (int)b;
  }

  // TCHECK: define void @__omp_offloading_{{.+}}([[S1]]* [[TH:%.+]], i{{[0-9]+}} [[B_IN:%.+]])
  // TCHECK:  [[TH_ADDR:%.+]] = alloca [[S1]]*,
  // TCHECK:  [[B_ADDR:%.+]] = alloca i{{[0-9]+}},

  // TCHECK:  store [[S1]]* [[TH]], [[S1]]** [[TH_ADDR]],
  // TCHECK:  store i{{[0-9]+}} [[B_IN]], i{{[0-9]+}}* [[B_ADDR]],
  // TCHECK:  [[TH_ADDR_REF:%.+]] = load [[S1]]*, [[S1]]** [[TH_ADDR]],
  // TCHECK-64:  [[B_ADDR_CONV:%.+]] = bitcast i{{[0-9]+}}* [[B_ADDR]] to i{{[0-9]+}}*

  // this->a = (double)b + 1.5; (only check access to b)
  // TCHECK-64:  [[B_IN_VAL:%.+]] = load i{{[0-9]+}}, i{{[0-9]+}}* [[B_ADDR_CONV]],

  // TCHECK: ret void
};



int bar(int n, double *ptr){
  int a = 0;
  a += foo(n, ptr);
  S1 S;
  a += S.r1(n);
  a += fstatic(n);
  a += ftemplate<int>(n);

  return a;
}

// template

// TCHECK: define void @__omp_offloading_{{.+}}(i{{[0-9]+}} [[A_IN:%.+]], [10 x i{{[0-9]+}}]*{{.+}} [[B_IN:%.+]])
// TCHECK:  [[A_ADDR:%.+]] = alloca i{{[0-9]+}},
// TCHECK:  [[B_ADDR:%.+]] = alloca [10 x i{{[0-9]+}}]*,
// TCHECK:  store i{{[0-9]+}} [[A_IN]], i{{[0-9]+}}* [[A_ADDR]],
// TCHECK:  store [10 x i{{[0-9]+}}]* [[B_IN]], [10 x i{{[0-9]+}}]** [[B_ADDR]],
// TCHECK-64:  [[A_ADDR_CONV:%.+]] = bitcast i{{[0-9]+}}* [[A_ADDR]] to i{{[0-9]+}}*
// TCHECK:  [[B_ADDR_REF:%.+]] = load [10 x i{{[0-9]+}}]*, [10 x i{{[0-9]+}}]** [[B_ADDR]],

// a += 1
// TCHECK-64:  [[A_IN_VAL:%.+]] = load i{{[0-9]+}}, i{{[0-9]+}}* [[A_ADDR_CONV]]
// TCHECK-32:  [[A_IN_VAL:%.+]] = load i{{[0-9]+}}, i{{[0-9]+}}* [[A_ADDR]]
// TCHECK-64:  store i{{[0-9]+}} {{.+}}, i{{[0-9]+}}* [[A_ADDR_CONV]],
// TCHECK-32:  store i{{[0-9]+}} {{.+}}, i{{[0-9]+}}* [[A_ADDR]],

// b[2] += 1
// TCHECK:  [[B_GEP:%.+]] = getelementptr inbounds [10 x i{{[0-9]+}}], [10 x i{{[0-9]+}}]* [[B_ADDR_REF]],
// TCHECK:  store i{{[0-9]+}} {{.+}}, i{{[0-9]+}}* [[B_GEP]],

// TCHECK: ret void

#endif
