#include "array_operator.h"
#include <amp.h>
using namespace std;
using namespace concurrency;


array_wrapper::array_wrapper(arraykd src, shared_ptr<array_operator> &op){
    //this->m_arraykd = src;
    //this->m_op = op;
    //this->m_array_ptr = 0;
    //if (this->m_op->use_amp()) {
    //    float* data = src.data();
    //    int dim = src.dim();
    //    if (dim == 1) {
    //        m_array_ptr = new array<float, 1>(src.size(), data);
    //    }
    //}
}

array_wrapper::~array_wrapper(){

}

void array_operator::conv(const array3d& src, const array4d& kernel, int stride, array3d& dst){
    CHECK(false);
}

void array_operator::deconv(const array3d& src, const array4d& kernel, int stride, array3d& dst){
    CHECK(false);
}

void array_operator::mul_addv(const array2d& A, const arraykd& B, arraykd& C){
    CHECK(A.cols() == B.size());
    CHECK(A.rows() == C.size());

    array_view<const float, 2> a(A.rows(), A.cols(), A.data());
    array_view<const float, 1> b(B.size(), B.data());
    array_view<float, 1> c(C.size(), C.data());

    const int cols = A.cols();
    parallel_for_each(c.extent, [=](index<1> idx) restrict(amp)
    {
        for (int i = 0; i < 2000; ++i){
            int row = idx[0];
            float sum = 0;
            for (int i = 0; i < cols; ++i) {
                sum += a(row, i)*b(i);
            }
            c(idx) += sum;
        }
    });

}

array_operator_ptr get_default_array_operator(){
    static array_operator_ptr ptr(0);
    //this var is for debug
#ifdef _DEBUG
    const bool try_use_gpu = true;
#else
    const bool try_use_gpu = true;
#endif

    if (!ptr){
        auto accname = concurrency::accelerator::default_accelerator;
        static auto acc = concurrency::accelerator(accname);
        if (acc.is_emulated || !try_use_gpu){
            cout << "cpu mode" << endl;
            ptr = array_operator_ptr(new array_operator());
        }
        else{
            wcout << acc.description << " detected!" << endl;
            ptr = array_operator_ptr(new array_operator(&acc.default_view));
        }
    }
    return ptr;
}

