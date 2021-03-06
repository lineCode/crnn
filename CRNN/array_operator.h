#ifndef ARRAY_OPERATOR_H
#define ARRAY_OPERATOR_H

#include "memory.h"

//provide a view for gpu or cpu operator
namespace Concurrency{
    class accelerator_view;
}

class array_operator;

class array_wrapper {
public:
    array_wrapper(arraykd src, std::shared_ptr<array_operator> &op);
    ~array_wrapper();

private:
    arraykd m_arraykd;
    std::shared_ptr<array_operator> m_op;
    void *m_array_ptr;
};

class array_operator {
public:
    array_operator(){ this->m_accelerator_ptr = 0; }
    array_operator(Concurrency::accelerator_view* acc){
        this->m_accelerator_ptr = acc;
    }

    //functions
    void conv(const array3d& src, const array4d& kernel, int stride, array3d& dst);
    void deconv(const array3d& src, const array4d& kernel, int stride, array3d& dst);

    void mul_addv(const array2d& A, const arraykd& B, arraykd& C);

    inline bool use_amp() {
        return this->m_accelerator_ptr != 0;
    }

private:
    Concurrency::accelerator_view *m_accelerator_ptr;
    bool m_use_amp;
};

typedef std::shared_ptr<array_operator> array_operator_ptr;

array_operator_ptr get_default_array_operator();

#endif
