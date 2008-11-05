#include"config.h"
#include<dune/istl/bvector.hh>
#include<dune/common/fvector.hh>

template<typename T, int BS>
void assign(Dune::FieldVector<T,BS>& b, const T& i)
{
  
  for(int j=0; j < BS; j++)
	  b[j] = i;
}

template<int BS>
int testVector()
{
    
  typedef Dune::FieldVector<int,BS> VectorBlock;
  typedef Dune::BlockVector<VectorBlock> Vector;
  typedef typename Vector::size_type size_type;
  
  // empty vector
  Vector v, w, v1(20), v2(20,100);
  v.reserve(100);
  assert(100==v.capacity());
  assert(20==v1.capacity());
  assert(100==v2.capacity());
  assert(20==v1.N());
  assert(20==v2.N());
  
  v.resize(25);
  
  assert(25==v.N());
  
  for(typename Vector::size_type i=0; i < v.N(); ++i)
    v[i] = i;
  
  for(typename Vector::size_type  i=0; i < v2.N(); ++i)
    v2[i] = i*10;
  w = v;
  

  assert(w.N()==v.N());
  assert(w.capacity()==v.capacity());

  for(typename Vector::size_type  i=0; i < v.N(); ++i)
    assert(v[i] == w[i]);

  w = static_cast<const Dune::block_vector_unmanaged<VectorBlock>&>(v);
  
  for(typename Vector::size_type  i=0; i < w.N(); ++i)
    assert(v[i] == w[i]);

  Vector z(w);
  
  assert(w.N()==z.N());
  assert(w.capacity()==z.capacity());

  for(typename Vector::size_type  i=0; i < w.N(); ++i)
    assert(z[i] == w[i]);

  Vector z1(static_cast<const Dune::block_vector_unmanaged<VectorBlock>&>(v2));

  assert(v2.N()==z1.N());
  assert(v2.capacity()==z1.capacity());

  for(typename Vector::size_type  i=1; i < v2.N(); ++i){
    assert(z1[i] == v2[i]);
  }
  
  v.reserve(150);
  assert(150==v.capacity());
  assert(25==v.N());

  VectorBlock b;
  
  // check the entries
  for(typename Vector::size_type  i=0; i < v.N(); ++i){
    assign(b, (int)i);
    assert(v[i] == b);
  }

  // Try to shrink the vector
  v.reserve(v.N());
  
  assert(v.N()==v.capacity());

  // check the entries

  for(typename Vector::size_type  i=0; i < v.N(); ++i){
    assign(b,(int)i);
    assert(v[i] == b);
  }
  
  return 0;
}


int main()
{
  typedef std::complex<double> value_type;
  //typedef double value_type;
  typedef Dune::FieldVector<value_type,1> VectorBlock;
  typedef Dune::BlockVector<VectorBlock> Vector;
  Vector v;
  v=0;
  Dune::BlockVector<Dune::FieldVector<std::complex<double>,1> > v1;
  v1=0;

  int ret = testVector<1>();
  return ret + testVector<3>();
}

