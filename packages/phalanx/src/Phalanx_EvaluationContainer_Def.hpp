#ifndef PHX_SCALAR_CONTAINER_DEF_HPP
#define PHX_SCALAR_CONTAINER_DEF_HPP

#include "Phalanx_Evaluator.hpp"
#include "Phalanx_DebugStrings.hpp"

// *************************************************************************
template <typename EvalT, typename Traits>
PHX::EvaluationContainer<EvalT, Traits>::EvaluationContainer()
{
  this->vp_manager_.setEvaluationTypeName( PHX::getTypeString<EvalT, Traits>() );
  this->data_container_template_manager_.buildObjects();
}

// *************************************************************************
template <typename EvalT, typename Traits> 
PHX::EvaluationContainer<EvalT, Traits>::~EvaluationContainer()
{

}

// *************************************************************************
template <typename EvalT, typename Traits>
void PHX::EvaluationContainer<EvalT, Traits>::
requireField(const PHX::FieldTag& f)
{
  this->vp_manager_.requireField(f);
}

// *************************************************************************
template <typename EvalT, typename Traits>
void PHX::EvaluationContainer<EvalT, Traits>::
registerEvaluator(const Teuchos::RCP<PHX::Evaluator<Traits> >& p)
{
  this->vp_manager_.registerEvaluator(p);
}

// *************************************************************************
template <typename EvalT, typename Traits> 
void PHX::EvaluationContainer<EvalT, Traits>::
postRegistrationSetup(std::size_t max_num_cells,
		      PHX::FieldManager<Traits>& fm)
{
  // Figure out all evaluator dependencies
  if ( !(this->vp_manager_.sortingCalled()) )
    this->vp_manager_.sortAndOrderEvaluators();

  // Determine total amount of memory for all variables
  allocator_.reset();
  
  const std::vector< Teuchos::RCP<PHX::FieldTag> >& var_list = 
    this->vp_manager_.getFieldTags();

  std::vector< Teuchos::RCP<PHX::FieldTag> >::const_iterator  var = 
    var_list.begin();
  for (; var != var_list.end(); ++var) {

    typename DCTM::iterator it = data_container_template_manager_.begin();
    for (; it != data_container_template_manager_.end(); ++it) {
      
      if ((*var)->dataTypeInfo() == it->dataTypeInfo()) {
	std::size_t size_of_data_type = it->getSizeOfDataType();
	int num_elements = max_num_cells * (*var)->dataLayout().size();
	allocator_.addRequiredChunk(size_of_data_type, num_elements);
      }
    }
  }

  allocator_.setup();

  // Allocate field data arrays
  //std::vector<PHX::FieldTag>::const_iterator  var = var_list.begin();
  for (var = var_list.begin(); var != var_list.end(); ++var) {

    typename DCTM::iterator it = data_container_template_manager_.begin();
    for (; it != data_container_template_manager_.end(); ++it) {
      
      if ((*var)->dataTypeInfo() == it->dataTypeInfo()) {
	it->allocateField(*var, max_num_cells, allocator_);
      }
    }
  }

  // Allow field evaluators to grab pointers to relevant field data
  this->vp_manager_.postRegistrationSetup(fm);
}

// *************************************************************************
template <typename EvalT, typename Traits>
void PHX::EvaluationContainer<EvalT, Traits>::
evaluateFields(typename Traits::EvalData d)
{
  this->vp_manager_.evaluateFields(d);
}

// *************************************************************************
template <typename EvalT, typename Traits>
void PHX::EvaluationContainer<EvalT, Traits>::
preEvaluate(typename Traits::PreEvalData d)
{
  this->vp_manager_.preEvaluate(d);
}

// *************************************************************************
template <typename EvalT, typename Traits>
void PHX::EvaluationContainer<EvalT, Traits>::
postEvaluate(typename Traits::PostEvalData d)
{
  this->vp_manager_.postEvaluate(d);
}

// *************************************************************************
template <typename EvalT, typename Traits> template <typename DataT>
Teuchos::ArrayRCP<DataT> 
PHX::EvaluationContainer<EvalT, Traits>::getFieldData(const PHX::FieldTag& f)
{
  Teuchos::ArrayRCP<DataT> r = 
    data_container_template_manager_.template getAsObject<DataT>()->
    getFieldData(f);
  return r;
}


// *************************************************************************
template <typename EvalT, typename Traits>
void PHX::EvaluationContainer<EvalT, Traits>::print(std::ostream& os) const
{
  std::string type = PHX::getTypeString<EvalT, Traits>();

  os << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << std::endl;
  os << "Starting PHX::EvaluationContainer Output" << std::endl;
  os << "Evaluation Type = " << type << std::endl;
  os << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << std::endl;
  os << this->vp_manager_ << std::endl;
  typename DCTM::const_iterator it = data_container_template_manager_.begin();
  for (; it != data_container_template_manager_.end(); ++it)
    os << *it << std::endl;
  os << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << std::endl;
  os << "Finished PHX::EvaluationContainer Output" << std::endl;
  os << "Evaluation Type = " << type << std::endl;
  os << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << std::endl;
  os << std::endl;
}

// *************************************************************************

#endif 
