#include"config.h"
#include<dune/istl/vbvector.hh>
#include<dune/common/fvector.hh>

using namespace Dune;

int main()
{
    VariableBlockVector<FieldVector<double,1> > v1;
    VariableBlockVector<FieldVector<double,1> > v2 = v1;
    VariableBlockVector<FieldVector<double,1> > v3(10);
    VariableBlockVector<FieldVector<double,1> > v4(10,4);

    v3.resize(20);
    v4.resize(20,8);

    v3 = v4;

    VariableBlockVector<FieldVector<double,1> >::CreateIterator cIt = v1.createbegin();

    for (; cIt!=v1.createend(); ++cIt){
      int foo = 0; ++foo;
    }
    

}

