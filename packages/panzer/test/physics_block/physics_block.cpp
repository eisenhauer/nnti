#include <Teuchos_ConfigDefs.hpp>
#include <Teuchos_UnitTestHarness.hpp>
#include <Teuchos_RCP.hpp>
#include <Teuchos_TimeMonitor.hpp>

#include "Phalanx_FieldManager.hpp"

#include "Panzer_Traits.hpp"
#include "Panzer_CellData.hpp"
#include "Panzer_InputEquationSet.hpp"
#include "Panzer_PhysicsBlock.hpp"
#include "Panzer_EpetraLinearObjFactory.hpp"

#include "user_app_EquationSetFactory.hpp"
#include "Panzer_ClosureModel_Factory_TemplateManager.hpp"
#include "user_app_ClosureModel_Factory_TemplateBuilder.hpp"
#include "user_app_ClosureModel_Factory.hpp"
#include "UnitTest_UniqueGlobalIndexer.hpp"

#include "Epetra_MpiComm.h"

namespace panzer_test_utils {

  Teuchos::RCP<panzer::PhysicsBlock> createPhysicsBlock()
  {
    
    panzer::InputEquationSet ies_1;
    {
      ies_1.name = "Energy";
      ies_1.basis = "Q2";
      ies_1.integration_order = 1;
      ies_1.model_id = "solid";
      ies_1.prefix = "";
    }
    
    panzer::InputEquationSet ies_2;
    {
      ies_2.name = "Energy";
      ies_2.basis = "Q1";
      ies_2.integration_order = 1;
      ies_2.model_id = "ion solid";
      ies_2.prefix = "ION_";
    }
    
    std::size_t num_cells = 20;
    std::size_t base_cell_dimension = 3;
    panzer::CellData cd(num_cells, base_cell_dimension);

    panzer::InputPhysicsBlock ipb;
    {
      ipb.physics_block_id = "4";
      ipb.eq_sets.push_back(ies_1);
      ipb.eq_sets.push_back(ies_2);
    }
    
    user_app::MyFactory eqs_factory;
    
    std::string element_block_id = "eblock_id";
    Teuchos::RCP<panzer::PhysicsBlock> physics_block = 
      Teuchos::rcp(new panzer::PhysicsBlock(ipb,element_block_id,cd,eqs_factory, false));
    
    return physics_block;
  }

  Teuchos::RCP<panzer::ClosureModelFactory_TemplateManager<panzer::Traits> >
  buildModelFactory() 
  {
    user_app::MyModelFactory_TemplateBuilder builder;

    Teuchos::RCP<panzer::ClosureModelFactory_TemplateManager<panzer::Traits> > 
      model_factory = 
      Teuchos::rcp(new panzer::ClosureModelFactory_TemplateManager<panzer::Traits>);
    
    model_factory->buildObjects(builder);

    return model_factory;
  }

  Teuchos::RCP<Teuchos::ParameterList> buildModelDescriptors()
  {    
    Teuchos::RCP<Teuchos::ParameterList> p = Teuchos::rcp(new Teuchos::ParameterList("Closure Models"));
    {
      p->sublist("solid").sublist("SOURCE_TEMPERATURE").set<double>("Value",1.0);
      p->sublist("ion solid").sublist("SOURCE_ION_TEMPERATURE").set<double>("Value",1.0);
    }

    return p;
  }

}

namespace panzer {


  TEUCHOS_UNIT_TEST(physics_block, getDOFNames)
  {
    Teuchos::RCP<panzer::PhysicsBlock> physics_block = 
      panzer_test_utils::createPhysicsBlock();

    const std::vector<std::string>& dof_names = physics_block->getDOFNames();

    TEST_EQUALITY(dof_names.size(), 2);
    TEST_EQUALITY(dof_names[0], "TEMPERATURE");
    TEST_EQUALITY(dof_names[1], "ION_TEMPERATURE");
  }

  TEUCHOS_UNIT_TEST(physics_block, getProvidedDOFs)
  {
    Teuchos::RCP<panzer::PhysicsBlock> physics_block = 
      panzer_test_utils::createPhysicsBlock();

    const std::vector<panzer::StrBasisPair>& basis = 
      physics_block->getProvidedDOFs();

    TEST_EQUALITY(basis.size(), 2);
    TEST_EQUALITY(basis[0].first, "TEMPERATURE");
    TEST_EQUALITY(basis[1].first, "ION_TEMPERATURE");
    TEST_EQUALITY(basis[0].second->name(), "Q2");
    TEST_EQUALITY(basis[1].second->name(), "Q1");
    TEST_EQUALITY(basis[0].second->getCardinality(), 27);
    TEST_EQUALITY(basis[1].second->getCardinality(), 8);
  }

  TEUCHOS_UNIT_TEST(physics_block, getBases)
  {
    Teuchos::RCP<panzer::PhysicsBlock> physics_block = 
      panzer_test_utils::createPhysicsBlock();

    const std::map<std::string,Teuchos::RCP<panzer::Basis> >& unique_basis = 
      physics_block->getBases();

    TEST_EQUALITY(unique_basis.size(), 2);
    TEST_ASSERT(unique_basis.find("Q2") != unique_basis.end());
    TEST_ASSERT(unique_basis.find("Q1") != unique_basis.end());
    TEST_EQUALITY(unique_basis.find("Q2")->second->getCardinality(), 27);
    TEST_EQUALITY(unique_basis.find("Q1")->second->getCardinality(), 8);
  }

  TEUCHOS_UNIT_TEST(physics_block, getBaseCellTopology)
  {
    Teuchos::RCP<panzer::PhysicsBlock> physics_block = 
      panzer_test_utils::createPhysicsBlock();
    
    TEST_EQUALITY(physics_block->getBaseCellTopology().getDimension(), 3);
  }

  TEUCHOS_UNIT_TEST(physics_block, physicsBlockID)
  {
    Teuchos::RCP<panzer::PhysicsBlock> physics_block = 
      panzer_test_utils::createPhysicsBlock();

    TEST_EQUALITY(physics_block->physicsBlockID(), "4");
  }

  TEUCHOS_UNIT_TEST(physics_block, getCellData)
  {
    Teuchos::RCP<panzer::PhysicsBlock> physics_block = 
      panzer_test_utils::createPhysicsBlock();

    TEST_EQUALITY(physics_block->cellData().numCells(), 20);
    TEST_EQUALITY(physics_block->cellData().isSide(), false);
  }

  TEUCHOS_UNIT_TEST(physics_block, nontemplate_evaluator_builders)
  {
    Teuchos::RCP<panzer::UniqueGlobalIndexer<short,int> > ugi 
          = Teuchos::rcp(new panzer::unit_test::UniqueGlobalIndexer(0,1));
    Teuchos::RCP<const Epetra_Comm> comm = Teuchos::rcp(new Epetra_MpiComm(MPI_COMM_WORLD));
    panzer::EpetraLinearObjFactory<panzer::Traits,short> elof(comm,ugi);

    Teuchos::RCP<panzer::PhysicsBlock> physics_block = 
      panzer_test_utils::createPhysicsBlock();

    PHX::FieldManager<panzer::Traits> fm;

    Teuchos::ParameterList user_data("User Data");

    physics_block->buildAndRegisterEquationSetEvaluators(fm, user_data);
    physics_block->buildAndRegisterGatherScatterEvaluators(fm, elof, user_data);

    Teuchos::RCP<panzer::ClosureModelFactory_TemplateManager<panzer::Traits> > factory =
      panzer_test_utils::buildModelFactory(); 

    Teuchos::RCP<Teuchos::ParameterList> models = panzer_test_utils::buildModelDescriptors();

    physics_block->buildAndRegisterClosureModelEvaluators(fm,*factory,*models, user_data);
  }

  TEUCHOS_UNIT_TEST(physics_block, elementBlockID)
  {

    Teuchos::RCP<panzer::PhysicsBlock> physics_block = 
      panzer_test_utils::createPhysicsBlock();
   

    TEST_EQUALITY(physics_block->elementBlockID(),"eblock_id");
  }

  TEUCHOS_UNIT_TEST(physics_block, templated_evaluator_builders)
  {
    Teuchos::RCP<panzer::UniqueGlobalIndexer<short,int> > ugi 
          = Teuchos::rcp(new panzer::unit_test::UniqueGlobalIndexer(0,1));
    Teuchos::RCP<const Epetra_Comm> comm = Teuchos::rcp(new Epetra_MpiComm(MPI_COMM_WORLD));
    panzer::EpetraLinearObjFactory<panzer::Traits,short> elof(comm,ugi);

    Teuchos::RCP<panzer::PhysicsBlock> physics_block = 
      panzer_test_utils::createPhysicsBlock();

    Teuchos::ParameterList user_data("User Data");

    PHX::FieldManager<panzer::Traits> fm;

    physics_block->buildAndRegisterEquationSetEvaluatorsForType<panzer::Traits::Residual>(fm, user_data);
    physics_block->buildAndRegisterGatherScatterEvaluatorsForType<panzer::Traits::Residual>(fm, elof, user_data);
    physics_block->buildAndRegisterEquationSetEvaluatorsForType<panzer::Traits::Jacobian>(fm, user_data);
    physics_block->buildAndRegisterGatherScatterEvaluatorsForType<panzer::Traits::Jacobian>(fm, elof, user_data);

    Teuchos::RCP<panzer::ClosureModelFactory_TemplateManager<panzer::Traits> > factory =
      panzer_test_utils::buildModelFactory(); 

    Teuchos::RCP<Teuchos::ParameterList> models = panzer_test_utils::buildModelDescriptors();

    physics_block->buildAndRegisterClosureModelEvaluatorsForType<panzer::Traits::Residual>(fm, *factory, *models, user_data);
    physics_block->buildAndRegisterClosureModelEvaluatorsForType<panzer::Traits::Jacobian>(fm, *factory, *models, user_data);
  }


}
