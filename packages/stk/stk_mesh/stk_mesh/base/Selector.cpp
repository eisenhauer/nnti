
#include <stdexcept>
#include <sstream>
#include <iostream>

#include <stk_mesh/base/Selector.hpp>
#include <stk_mesh/base/Bucket.hpp>
#include <stk_mesh/base/MetaData.hpp>
#include <stk_mesh/base/BulkData.hpp>
#include <stk_mesh/base/Types.hpp>

namespace stk {
namespace mesh {

Selector::Selector( )
  : m_mesh_meta_data(0), m_op()
{ 
  compoundAll();
}


Selector::~Selector( )
{ }


// Deep copy
Selector::Selector( const Selector & selector )
  : m_mesh_meta_data(selector.m_mesh_meta_data), m_op(selector.m_op)
{
}


Selector::Selector( const Part & p )
  : m_mesh_meta_data( & p.mesh_meta_data() ) , m_op()
{
  m_op.push_back( OpType( p.mesh_meta_data_ordinal() , 0 , 0 ) );
}

void Selector::compoundAll() 
{
  m_op.insert( m_op.begin(), OpType( 0, 0, m_op.size()+1 ) );
}


Selector & Selector::complement()
{
  bool singlePart = (m_op.size() == 1);
  bool fullCompoundPart = (m_op[0].m_count == m_op.size());

  if ( !(singlePart || fullCompoundPart) ) {
    // Turn into a compound 
    compoundAll();
  }
  // Flip the bit
  m_op[0].m_unary ^= 1; 
  return *this;
}


Selector & Selector::operator = ( const Selector & B )
{
  this->m_mesh_meta_data = B.m_mesh_meta_data;
  this->m_op = B.m_op;
  return *this;
}

Selector & Selector::operator &= ( const Selector & B )
{
  if (m_mesh_meta_data == 0) {
    m_mesh_meta_data = B.m_mesh_meta_data;
  }
  verify_compatible( B );
  m_op.insert( m_op.end() , B.m_op.begin() , B.m_op.end() );
  return *this;
}


Selector & Selector::operator |= ( const Selector & B )
{
  if (m_mesh_meta_data == 0) {
    m_mesh_meta_data = B.m_mesh_meta_data;
  }
  verify_compatible( B );

  Selector notB = B; notB.complement();

  // this UNION B == ! ( ! this & ! B )

  this->complement();                   //   ( ! (this) )

  const unsigned finalSize = 1 + m_op.size() + notB.m_op.size();

  m_op.insert( 
      m_op.end(), 
      notB.m_op.begin(), 
      notB.m_op.end() );                // ! ( ! (this) & !B )
  m_op.insert( 
      m_op.begin(), 
      OpType( 0 , 1 , finalSize ) );    // ! ( ! (this) & ? )
  return *this;
}


void Selector::verify_compatible( const Selector & B ) const
{
  if (B.m_mesh_meta_data != m_mesh_meta_data) {
    std::ostringstream msg;
    msg << "Selector = " << *this << " has mesh meta data pointer = " << m_mesh_meta_data << std::endl;
    msg << "Selector = " << B << " has mesh meta data pointer = " << B.m_mesh_meta_data << std::endl;
    msg << "These selectors contain incompatible mesh meta data pointers!";
    throw std::runtime_error( msg.str() );
  }
}


void Selector::verify_compatible( const Bucket & B ) const
{
  const MetaData * B_mesh_meta_data = &B.mesh().mesh_meta_data();
  if (B_mesh_meta_data != m_mesh_meta_data) {
    std::ostringstream msg;
    msg << "Selector = " << *this << " has mesh meta data pointer = " << m_mesh_meta_data << std::endl;
    msg << "Bucket has mesh meta data pointer = " << B_mesh_meta_data << std::endl;
    msg << "This selector is incompatible with this bucket!";
    throw std::runtime_error( msg.str() );
  }
}


bool Selector::apply( 
    unsigned part_id, 
    const Bucket & candidate 
    ) const
{
  // Search for 'part_id' in the bucket's list of sorted integer part ids
  return has_superset(candidate,part_id);
}

bool Selector::apply( 
    std::vector<OpType>::const_iterator i,
    std::vector<OpType>::const_iterator j,
    const Bucket & candidate 
    ) const
{
  bool result = i != j ;
  while ( result && i != j ) {
    if ( i->m_count ) { // Compound statement
      result = i->m_unary ^ apply( i + 1 , i + i->m_count , candidate );
      i += i->m_count ;
    }
    else { // Test for containment of bucket in this part, or not in
      result = i->m_unary ^ apply( i->m_part_id , candidate );
      ++i ;
    }
  }
  return result ;
}


bool Selector::operator()( const Bucket & candidate ) const
{
  if (m_mesh_meta_data != NULL) {
    verify_compatible(candidate);
  }
  return apply( m_op.begin() , m_op.end() , candidate );
}

Selector operator & ( const Part & A , const Part & B )
{ 
  Selector S( A ); 
  S &= Selector( B ); 
  return S; 
}


Selector operator & ( const Part & A , const Selector & B )
{ 
  Selector S( A ); 
  S &= B; 
  return S; 
}

Selector operator & ( const Selector & A, const Part & B )
{ 
  Selector S( A ); 
  S &= Selector(B); 
  return S; 
}

Selector operator & ( const Selector & A, const Selector & B )
{ 
  Selector S( A ); 
  S &= Selector(B); 
  return S; 
}

Selector operator | ( const Part & A , const Part & B )
{ 
  Selector S( A ); 
  S |= Selector( B ); 
  return S; 
}


Selector operator | ( const Part & A , const Selector & B )
{ 
  Selector S( A ); 
  S |= B; 
  return S; 
}

Selector operator | ( const Selector & A, const Part & B  )
{ 
  Selector S( A ); 
  S |= Selector(B); 
  return S; 
}

Selector operator | ( const Selector & A, const Selector & B  )
{ 
  Selector S( A ); 
  S |= Selector(B); 
  return S; 
}




Selector operator ! ( const Part & A )
{
  Selector S(A);
  return S.complement();
}


std::ostream & operator<<( std::ostream & out, const Selector & selector)
{
  out << selector.printExpression(selector.m_op.begin(),selector.m_op.end());
  return out;
}

std::string Selector::printExpression(
    const std::vector<OpType>::const_iterator start,
    const std::vector<OpType>::const_iterator finish
    ) const
{
  std::ostringstream outS;

  std::vector<OpType>::const_iterator start_it = start;
  std::vector<OpType>::const_iterator finish_it = finish;

  const OpType & op = *start_it;
  if (op.m_count > 0) { // Compound
    if (op.m_unary != 0) { // Complement
      outS << "!";
    } 
    outS << "(";
    if (op.m_count == 1) {
      outS << ")";
    }
    else {
      finish_it = start_it;
      for (int i=0 ; i < op.m_count ; ++i) {
        ++finish_it;
      }
      ++start_it;
      outS << printExpression(start_it,finish_it) << ")";
      start_it = finish_it;
      --start_it; // back up one
    }
  }
  else { // Part
    if (m_mesh_meta_data != NULL) {
      Part & part = m_mesh_meta_data->get_part(op.m_part_id);
      if (op.m_unary != 0) { // Complement
        outS << "!";
      }
      outS << part.name();
    }
  }
  ++start_it;
  if (start_it != finish) {
    outS << " AND " << printExpression(start_it,finish);
  }
  return outS.str();
}


Selector::OpType::OpType( const OpType & opType ) 
  : m_part_id(opType.m_part_id), 
    m_unary(opType.m_unary),
    m_count(opType.m_count)
{
}


Selector::OpType & Selector::OpType::operator=( const OpType & opType )
{
  this->m_part_id = opType.m_part_id;
  this->m_unary = opType.m_unary;
  this->m_count = opType.m_count;
  return *this;
}


Selector selectUnion( const PartVector& union_part_vector )
{
  Selector selector;
  if (union_part_vector.size() > 0) {
    selector = *union_part_vector[0];
    for (unsigned i = 1 ; i < union_part_vector.size() ; ++i) {
      selector |= *union_part_vector[i];
    }
  }
  return selector;
}


Selector selectIntersection( const PartVector& intersection_part_vector )
{
  Selector selector;
  if (intersection_part_vector.size() > 0) {
    selector = *intersection_part_vector[0];
    for (unsigned i = 1 ; i < intersection_part_vector.size() ; ++i) {
      selector &= *intersection_part_vector[i];
    }
  }
  return selector;
}



} // namespace mesh 
} // namespace stk 


