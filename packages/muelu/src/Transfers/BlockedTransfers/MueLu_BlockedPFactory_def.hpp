/*
 * MueLu_BlockedPFactory_def.hpp
 *
 *  Created on: 02.01.2012
 *      Author: tobias
 */

#ifndef MUELU_BLOCKEDPFACTORY_DEF_HPP_
#define MUELU_BLOCKEDPFACTORY_DEF_HPP_

#include "MueLu_BlockedPFactory_decl.hpp"
#include "MueLu_TentativePFactory.hpp"
#include "MueLu_FactoryBase.hpp"
#include "MueLu_SmootherFactory.hpp"
#include "MueLu_FactoryManager.hpp"
#include "MueLu_Utilities.hpp"
#include "MueLu_Monitor.hpp"
#include "MueLu_HierarchyHelpers.hpp"

#include "Xpetra_VectorFactory.hpp"
#include "Xpetra_ImportFactory.hpp"
#include "Xpetra_ExportFactory.hpp"

#include <Xpetra_BlockedCrsOperator.hpp>
#include <Xpetra_Map.hpp>
#include <Xpetra_MapFactory.hpp>
#include <Xpetra_MapExtractor.hpp>
#include <Xpetra_MapExtractorFactory.hpp>

namespace MueLu {

template <class Scalar,class LocalOrdinal, class GlobalOrdinal, class Node, class LocalMatOps>
BlockedPFactory<Scalar, LocalOrdinal, GlobalOrdinal, Node, LocalMatOps>::BlockedPFactory(RCP<FactoryBase> AFact)
: AFact_(AFact),
  diagonalView_("current") {

}

template <class Scalar,class LocalOrdinal, class GlobalOrdinal, class Node, class LocalMatOps>
BlockedPFactory<Scalar, LocalOrdinal, GlobalOrdinal, Node, LocalMatOps>::~BlockedPFactory() {}

template <class Scalar,class LocalOrdinal, class GlobalOrdinal, class Node, class LocalMatOps>
void BlockedPFactory<Scalar, LocalOrdinal, GlobalOrdinal, Node, LocalMatOps>::SetDiagonalView(std::string const& diagView) {
  diagonalView_ = diagView;
}

template <class Scalar,class LocalOrdinal, class GlobalOrdinal, class Node, class LocalMatOps>
std::string BlockedPFactory<Scalar, LocalOrdinal, GlobalOrdinal, Node, LocalMatOps>::GetDiagonalView() {
  return diagonalView_;
}

template <class Scalar,class LocalOrdinal, class GlobalOrdinal, class Node, class LocalMatOps>
void BlockedPFactory<Scalar, LocalOrdinal, GlobalOrdinal, Node, LocalMatOps>::AddFactoryManager(RCP<const FactoryManagerBase> FactManager) {
  FactManager_.push_back(FactManager);
}

template <class Scalar,class LocalOrdinal, class GlobalOrdinal, class Node, class LocalMatOps>
void BlockedPFactory<Scalar, LocalOrdinal, GlobalOrdinal, Node, LocalMatOps>::DeclareInput(Level &fineLevel, Level &coarseLevel) const {
  fineLevel.DeclareInput("A",AFact_.get(),this);

  //Teuchos::RCP<Teuchos::FancyOStream> fos = Teuchos::getFancyOStream(Teuchos::rcpFromRef(std::cout));

  std::vector<Teuchos::RCP<const FactoryManagerBase> >::const_iterator it;
  for(it = FactManager_.begin(); it!=FactManager_.end(); it++) {
    SetFactoryManager fineSFM  (rcpFromRef(fineLevel),   *it);
    SetFactoryManager coarseSFM(rcpFromRef(coarseLevel), *it);

    if (!restrictionMode_)
      coarseLevel.DeclareInput("P",(*it)->GetFactory("P").get(), this);
    else
      coarseLevel.DeclareInput("R",(*it)->GetFactory("R").get(), this);
  }

}

template <class Scalar,class LocalOrdinal, class GlobalOrdinal, class Node, class LocalMatOps>
void BlockedPFactory<Scalar, LocalOrdinal, GlobalOrdinal, Node, LocalMatOps>::Build(Level& fineLevel, Level &coarseLevel) const {
  typedef Xpetra::Operator<Scalar, LocalOrdinal, GlobalOrdinal, Node, LocalMatOps> OperatorClass;
  typedef Xpetra::CrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal, Node, LocalMatOps> CrsMatrixClass;
  typedef Xpetra::CrsOperator<Scalar, LocalOrdinal, GlobalOrdinal, Node, LocalMatOps> CrsOperatorClass;
  typedef Xpetra::BlockedCrsOperator<Scalar, LocalOrdinal, GlobalOrdinal, Node, LocalMatOps> BlockedCrsOOperator;
  typedef Xpetra::Map<LocalOrdinal, GlobalOrdinal, Node> MapClass;
  typedef Xpetra::MapFactory<LocalOrdinal, GlobalOrdinal, Node> MapFactoryClass;
  typedef Xpetra::MapExtractor<Scalar, LocalOrdinal, GlobalOrdinal, Node> MapExtractorClass;
  typedef Xpetra::MapExtractorFactory<Scalar, LocalOrdinal, GlobalOrdinal, Node> MapExtractorFactoryClass;


  //Teuchos::RCP<Teuchos::FancyOStream> fos = Teuchos::getFancyOStream(Teuchos::rcpFromRef(std::cout));
  //std::ostringstream buf; buf << coarseLevel.GetLevelID();

  // Level Get
  RCP<Operator> A     = fineLevel.  Get< RCP<Operator> >("A", AFact_.get()); // IMPORTANT: use main factory manager for getting A
  RCP<BlockedCrsOOperator> bA = Teuchos::rcp_dynamic_cast<BlockedCrsOOperator>(A);
  TEUCHOS_TEST_FOR_EXCEPTION(bA==Teuchos::null, Exceptions::BadCast, "MueLu::BlockedPFactory::Build: input matrix A is not of type BlockedCrsOperator! error.");

  // plausibility check
  TEUCHOS_TEST_FOR_EXCEPTION(bA->Rows() != FactManager_.size(), Exceptions::RuntimeError, "MueLu::BlockedPFactory::Build: number of block rows of A does not match number of SubFactoryManagers. error.");
  TEUCHOS_TEST_FOR_EXCEPTION(bA->Cols() != FactManager_.size(), Exceptions::RuntimeError, "MueLu::BlockedPFactory::Build: number of block cols of A does not match number of SubFactoryManagers. error.");

  // build blocked prolongator
  std::vector<RCP<Operator> > subBlockP;
  std::vector<RCP<const MapClass> > subBlockPRangeMaps;
  std::vector<RCP<const MapClass    > > subBlockPDomainMaps;
  std::vector<GO> fullRangeMapVector;
  std::vector<GO> fullDomainMapVector;
  subBlockP.reserve(FactManager_.size());       // reserve size for block P operators
  subBlockPRangeMaps.reserve(FactManager_.size());       // reserve size for block P operators
  subBlockPDomainMaps.reserve(FactManager_.size());       // reserve size for block P operators

  // build and store the subblocks and the corresponding range and domain maps
  // since we put together the full range and domain map from the submaps we do not have
  // to use the maps from blocked A
  std::vector<Teuchos::RCP<const FactoryManagerBase> >::const_iterator it;
  for(it = FactManager_.begin(); it!=FactManager_.end(); it++) {
    SetFactoryManager fineSFM  (rcpFromRef(fineLevel),   *it);
    SetFactoryManager coarseSFM(rcpFromRef(coarseLevel), *it);
    if(!restrictionMode_)
      subBlockP.push_back(coarseLevel.Get<RCP<Operator> >("P", (*it)->GetFactory("P").get())); // create and return block P operator
    else
      subBlockP.push_back(coarseLevel.Get<RCP<Operator> >("R", (*it)->GetFactory("R").get())); // create and return block R operator
    subBlockPRangeMaps.push_back(subBlockP.back()->getRangeMap());
    Teuchos::ArrayView< const GlobalOrdinal > nodeRangeMap = subBlockPRangeMaps.back()->getNodeElementList();
    fullRangeMapVector.insert(fullRangeMapVector.end(), nodeRangeMap.begin(), nodeRangeMap.end());
    subBlockPDomainMaps.push_back(subBlockP.back()->getDomainMap());
    Teuchos::ArrayView< const GlobalOrdinal > nodeDomainMap = subBlockPDomainMaps.back()->getNodeElementList();
    fullDomainMapVector.insert(fullDomainMapVector.end(), nodeDomainMap.begin(), nodeDomainMap.end());
  }

  // build full row (=range) map and full domain map from vector of prolongator objects

  // extract map index base from maps of blocked A
  GO rangeIndexBase  = 0;
  GO domainIndexBase = 0;
  if(!restrictionMode_) { // prolongation mode: just use index base of range and domain map of bA
    rangeIndexBase = bA->getRangeMap()->getIndexBase();
    domainIndexBase= bA->getDomainMap()->getIndexBase();
  } else { // restriction mode: switch range and domain map for blocked restriction operator
    rangeIndexBase = bA->getDomainMap()->getIndexBase();
    domainIndexBase= bA->getRangeMap()->getIndexBase();
  }

  Teuchos::ArrayView<GO> fullRangeMapGIDs(&fullRangeMapVector[0],fullRangeMapVector.size());
  RCP<const MapClass > fullRangeMap =
    MapFactoryClass::Build(
                      bA->getRangeMap()->lib(),
                      Teuchos::OrdinalTraits<Xpetra::global_size_t>::invalid(),
                      fullRangeMapGIDs,
                      rangeIndexBase,
                      bA->getRangeMap()->getComm()
                      );
  Teuchos::ArrayView<GO> fullDomainMapGIDs(&fullDomainMapVector[0],fullDomainMapVector.size());
  RCP<const MapClass > fullDomainMap =
    MapFactoryClass::Build(
                      bA->getDomainMap()->lib(),
                      Teuchos::OrdinalTraits<Xpetra::global_size_t>::invalid(),
                      fullDomainMapGIDs,
                      domainIndexBase,
                      bA->getDomainMap()->getComm()
                      );

  // build map extractors
  Teuchos::RCP<const MapExtractorClass> rangeMapExtractor  = MapExtractorFactoryClass::Build(fullRangeMap, subBlockPRangeMaps);
  Teuchos::RCP<const MapExtractorClass> domainMapExtractor = MapExtractorFactoryClass::Build(fullDomainMap, subBlockPDomainMaps);

  Teuchos::RCP<BlockedCrsOOperator> bP = Teuchos::rcp(new BlockedCrsOOperator(rangeMapExtractor,domainMapExtractor,10));
  for(size_t i = 0; i<subBlockPRangeMaps.size(); i++) {
    Teuchos::RCP<CrsOperatorClass> crsOpii = Teuchos::rcp_dynamic_cast<CrsOperatorClass>(subBlockP[i]);
    Teuchos::RCP<CrsMatrixClass> crsMatii = crsOpii->getCrsMatrix();
    bP->setMatrix(i,i,crsMatii);
  }

  bP->fillComplete();

  //bP->describe(*fos,Teuchos::VERB_EXTREME);

  // Level Set
  if(!restrictionMode_)
  {
    // prolongation factory is in prolongation mode
    coarseLevel.Set("P", Teuchos::rcp_dynamic_cast<OperatorClass>(bP), this);
  }
  else
  {
    // prolongation factory is in restriction mode
    // we do not have to transpose the blocked R operator since the subblocks on the diagonal
    // are already valid R subblocks
    coarseLevel.Set("R", Teuchos::rcp_dynamic_cast<OperatorClass>(bP), this);
  }

}

template <class Scalar,class LocalOrdinal, class GlobalOrdinal, class Node, class LocalMatOps>
void BlockedPFactory<Scalar, LocalOrdinal, GlobalOrdinal, Node, LocalMatOps>::BuildP(Level& fineLevel, Level &coarseLevel) const {

}

} // namespace MueLu

#endif /* MUELU_BLOCKEDPFACTORY_DEF_HPP_ */
