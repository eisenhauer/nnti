// @HEADER
//***********************************************************************
//                Copyright message goes here. 
// ***********************************************************************
// @HEADER

/*! \file Zoltan2_AlgRCB.hpp
    \brief Contains the recursive coordinate bisection algorthm.
*/

#ifndef _ZOLTAN2_ALGPQJagged_HPP_
#define _ZOLTAN2_ALGPQJagged_HPP_

#include <Zoltan2_AlgRCB_methods.hpp>
#include <Zoltan2_CoordinateModel.hpp>
#include <Zoltan2_Metric.hpp>             // won't need this
#include <Zoltan2_GetParameter.hpp>

#include <Teuchos_ParameterList.hpp>
#include <omp.h>

#define imbalanceOf(Wachieved, totalW, expectedRatio) \
	(Wachieved) / ((totalW) * (expectedRatio)) - 1


namespace Zoltan2{


//#define RCBCODE


template <typename lno_t, typename size_tt>
class pqJagged_PartVertices{
private:
	size_tt vertexCount;
	lno_t *linkedList; //initially filled with -1's.
	lno_t *partBegins; //initially filled with -1's.
	lno_t *partEnds; //initially filled with -1's.
public:

	pqJagged_PartVertices(){};
	void set(const size_tt &vertexCount_, lno_t *linkedList_, lno_t *partBegins_, lno_t *partEnds_){
		vertexCount = vertexCount_;
		linkedList = linkedList_;
		partBegins = partBegins_;
		partEnds = partEnds_;
	}

	//user is responsible from providing the correct number of part counts
	void inserToPart (int partNo, lno_t vertexIndex){
		switch (partEnds[partNo]){
		case -1: // this means partBegins[partNo] is also -1.
			partBegins[partNo] = vertexIndex;
			partEnds[partNo] = vertexIndex;
			break;
		default:
			linkedList[vertexIndex] = partBegins[partNo];
			partBegins[partNo] = vertexIndex;
			break;
		}
	}

	lno_t *getLinkedList(){ return linkedList;}
	lno_t *getPartBegins(){ return partBegins;}
	lno_t *getPartEnds(){ return partEnds;}

};






template <typename scalar_t>

scalar_t pivotPos (scalar_t * cutUpperBounds, scalar_t *cutLowerBounds,size_t currentLine, scalar_t *cutUpperWeight, scalar_t *cutLowerWeight, scalar_t ew){

	return ((cutUpperBounds[currentLine] - cutLowerBounds[currentLine]) /
			(cutUpperWeight[currentLine] - cutLowerWeight[currentLine]))  * (ew - cutLowerWeight[currentLine]) + cutLowerBounds[currentLine];
}


/*
 *Partitioning problem parameters of interest:
 *	Partitioning objective
 *	imbalance_tolerance
 *
 *Geometric partitioning problem parameters of interest:
 *	average_cuts
 *	rectilinear_blocks
 *	bisection_num_test_cuts (experimental)
 */
template <typename T>
void pqJagged_getParameters(const Teuchos::ParameterList &pl, T &imbalanceTolerance,
		multiCriteriaNorm &mcnorm, std::bitset<NUM_RCB_PARAMS> &params,  int &numTestCuts, bool &ignoreWeights){

	bool isSet;
	string strChoice;
	int intChoice;


	getParameterValue<string>(pl, "partitioning",
			"objective", isSet, strChoice);

	if (isSet && strChoice == string("balance_object_count"))
		params.set(rcb_balanceCount);
	else if (isSet && strChoice ==
			string("multicriteria_minimize_total_weight")){
		params.set(rcb_minTotalWeight);
		mcnorm = normMinimizeTotalWeight;
	}
	else if (isSet && strChoice ==
			string("multicriteria_minimize_maximum_weight")){
		params.set(rcb_minMaximumWeight);
		mcnorm = normMinimizeMaximumWeight;
	}
	else if (isSet && strChoice ==
			string("multicriteria_balance_total_maximum")){
		params.set(rcb_balanceTotalMaximum);
		mcnorm = normBalanceTotalMaximum;
	}
	else{
		params.set(rcb_balanceWeight);
		mcnorm = normBalanceTotalMaximum;
	}

	double tol;
	getParameterValue<double>(pl, "partitioning",
			"imbalance_tolerance", isSet, tol);

	if (!isSet)
		imbalanceTolerance = .1;
	else
		imbalanceTolerance = tol - 1.0;

	if (imbalanceTolerance <= 0)
		imbalanceTolerance = 10e-4;  // TODO - what's a good choice

	////////////////////////////////////////////////////////
	// Geometric partitioning problem parameters of interest:
	//    average_cuts
	//    rectilinear_blocks
	//    bisection_num_test_cuts (experimental)

	getParameterValue<int>(pl, "partitioning", "geometric",
			"average_cuts", isSet, intChoice);

	if (isSet && intChoice==1)
		params.set(rcb_averageCuts);

	getParameterValue<int>(pl, "partitioning", "geometric",
			"rectilinear_blocks", isSet, intChoice);

	if (isSet && intChoice==1)
		params.set(rcb_rectilinearBlocks);

	getParameterValue<int>(pl, "partitioning", "geometric",
			"bisection_num_test_cuts", isSet, intChoice);
	if (isSet)
		numTestCuts = intChoice;
	ignoreWeights = params.test(rcb_balanceCount);


}

template <typename Adapter>
void pqJagged_getCoordinateValues( const RCP<const CoordinateModel<
	    typename Adapter::base_adapter_t> > &coords, int &coordDim,
	    int &weightDim, size_t &numLocalCoords, global_size_t &numGlobalCoords, int &criteriaDim, const bool &ignoreWeights){

	coordDim = coords->getCoordinateDim();
	weightDim = coords->getCoordinateWeightDim();
	numLocalCoords = coords->getLocalNumCoordinates();
	numGlobalCoords = coords->getGlobalNumCoordinates();
	criteriaDim = (weightDim ? weightDim : 1);
	if (criteriaDim > 1 && ignoreWeights)
		criteriaDim = 1;
}

template <typename Adapter, typename scalar_t, typename gno_t>
void pqJagged_getInputValues(
		const RCP<const Environment> &env, const RCP<const CoordinateModel<
	    typename Adapter::base_adapter_t> > &coords,
	    RCP<PartitioningSolution<Adapter> > &solution,
	    std::bitset<NUM_RCB_PARAMS> &params,
	    const int &coordDim,
	    const int &weightDim,
	    const size_t &numLocalCoords, const global_size_t &numGlobalCoords, size_t &numGlobalParts, int &pqJagged_multiVectorDim,
	    scalar_t **pqJagged_values, const int &criteriaDim, scalar_t **pqJagged_weights, gno_t * &pqJagged_gnos, bool &ignoreWeights,
	    bool *pqJagged_uniformWeights, bool *pqJagged_uniformParts, scalar_t **pqJagged_partSizes
#ifdef RCBCODE
		,ArrayRCP<bool> &uniformWeights,
		RCP< Tpetra::MultiVector<scalar_t, lno_t, gno_t, node_t> > &mvector,
		int &multiVectorDim,
		Array<ArrayRCP<scalar_t> > &partSizes,
	    RCP<Comm<int> > comm
#endif

){



	typedef typename Adapter::node_t node_t;
	typedef typename Adapter::lno_t lno_t;
	typedef StridedData<lno_t, scalar_t> input_t;

	ArrayView<const gno_t> gnos;
	ArrayView<input_t>     xyz;
	ArrayView<input_t>     wgts;

	coords->getCoordinates(gnos, xyz, wgts);
	pqJagged_gnos =(gno_t *)gnos.getRawPtr();
#ifdef RCBCODE
	Array<ArrayRCP<const scalar_t> > values(coordDim);
#endif


	for (int dim=0; dim < coordDim; dim++){
		ArrayRCP<const scalar_t> ar;
		xyz[dim].getInputArray(ar);
#ifdef RCBCODE
		values[dim] = ar;
#endif
		//pqJagged coordinate values assignment
		pqJagged_values[dim] =  (scalar_t *)ar.getRawPtr();
	}




#ifdef RCBCODE
	Array<ArrayRCP<const scalar_t> > weights(criteriaDim);
#endif




	if (weightDim == 0 || ignoreWeights){

#ifdef RCBCODE
		uniformWeights[0] = true;
#endif

		pqJagged_uniformWeights[0] = true;
	}
	else{
		for (int wdim = 0; wdim < weightDim; wdim++){
			if (wgts[wdim].size() == 0){
#ifdef RCBCODE
				uniformWeights[wdim] = true;
#endif
				pqJagged_uniformWeights[wdim] = true;
			}
			else{

#ifdef RCBCODE
				uniformWeights[wdim] = false;
#endif
				ArrayRCP<const scalar_t> ar;
				wgts[wdim].getInputArray(ar);
#ifdef RCBCODE
				weights[wdim] = ar;
#endif
				pqJagged_uniformWeights[wdim] = false;
				pqJagged_weights[wdim] = (scalar_t *) ar.getRawPtr();
			}
		}
	}

#ifdef RCBCODE
	if (env->doStatus() && (numGlobalCoords < 500)){
		ostringstream oss;
		oss << "Problem: ";
		for (size_t i=0; i < numLocalCoords; i++){
			oss << gnos[i] << " (";
			for (int dim=0; dim < coordDim; dim++)
				oss << (xyz[dim])[i] << " ";
			oss << ") ";
		}

		env->debug(VERBOSE_DETAILED_STATUS, oss.str());
	}
#endif

	////////////////////////////////////////////////////////
	// From the Solution we get part information.
	// If the part sizes for a given criteria are not uniform,
	// then they are values that sum to 1.0.

	numGlobalParts = solution->getGlobalNumberOfParts();

#ifdef RCBCODE
	Array<bool> uniformParts(criteriaDim);
#endif


	for (int wdim = 0; wdim < criteriaDim; wdim++){
		if (solution->criteriaHasUniformPartSizes(wdim)){
#ifdef RCBCODE
			uniformParts[wdim] = true;
#endif
			pqJagged_uniformParts[wdim] = true;
		}
		else{
			scalar_t *tmp = new scalar_t [numGlobalParts];
			env->localMemoryAssertion(__FILE__, __LINE__, numGlobalParts, tmp) ;

			for (size_t i=0; i < numGlobalParts; i++){
				tmp[i] = solution->getCriteriaPartSize(wdim, i);
			}
#ifdef RCBCODE
			partSizes[wdim] = arcp(tmp, 0, numGlobalParts);
#endif
			pqJagged_partSizes[wdim] = tmp;

		}
	}

	// It may not be possible to solve the partitioning problem
	// if we have multiple weight dimensions with part size
	// arrays that differ. So let's be aware of this possibility.

	bool multiplePartSizeSpecs = false;

	if (criteriaDim > 1){
		for (int wdim1 = 0; wdim1 < criteriaDim; wdim1++)
			for (int wdim2 = wdim1+1; wdim2 < criteriaDim; wdim2++)
				if (!solution->criteriaHaveSamePartSizes(wdim1, wdim2)){
					multiplePartSizeSpecs = true;
					break;
				}
	}

	if (multiplePartSizeSpecs)
		params.set(rcb_multiplePartSizeSpecs);

	////////////////////////////////////////////////////////
	// Create the distributed data for the algorithm.
	//
	// It is a multivector containing one vector for each coordinate
	// dimension, plus a vector for each weight dimension that is not
	// uniform.

#ifdef RCBCODE
	typedef Tpetra::Map<lno_t, gno_t, node_t> map_t;
	typedef Tpetra::MultiVector<scalar_t, lno_t, gno_t, node_t> mvector_t;
	multiVectorDim = coordDim;
#endif
	pqJagged_multiVectorDim = coordDim;
	for (int wdim = 0; wdim < criteriaDim; wdim++){
#ifdef RCBCODE
		if (!uniformWeights[wdim]) multiVectorDim++;
#endif
		if (!pqJagged_uniformWeights[wdim]) pqJagged_multiVectorDim++;
	}


#ifdef RCBCODE
	gno_t gnoMin, gnoMax;
	coords->getIdentifierMap()->getGnoRange(gnoMin, gnoMax);
	RCP<map_t> map;
	try{
		map = rcp(new map_t(numGlobalCoords, gnos, gnoMin, comm));
	}
	Z2_THROW_OUTSIDE_ERROR(*env)

	typedef ArrayView<const scalar_t> coordList_t;

	coordList_t *avList = new coordList_t [multiVectorDim];

	for (int dim=0; dim < coordDim; dim++)
		avList[dim] = values[dim].view(0, numLocalCoords);

	for (int wdim=0, idx=coordDim; wdim < criteriaDim; wdim++)
		if (!uniformWeights[wdim])
			avList[idx++] = weights[wdim].view(0, numLocalCoords);

	ArrayRCP<const ArrayView<const scalar_t> > vectors =
			arcp(avList, 0, multiVectorDim);



	try{
		mvector = rcp(new mvector_t(
				map, vectors.view(0, multiVectorDim), multiVectorDim));
	}
	Z2_THROW_OUTSIDE_ERROR(*env)

	env->timerStop(BOTH_TIMERS, "RCB set up");
#endif
}

template <typename scalar_t, typename gno_t>
void pqJagged_printInput(int coordDim, int weightDim, size_t numLocalCoords, global_size_t numGlobalCoords,
		int criteriaDim, scalar_t **pqJagged_values, scalar_t **pqJagged_weights,
		bool *pqJagged_uniformParts, bool *pqJagged_uniformWeights, gno_t *pqJagged_gnos,
		bool ignoreWeights,size_t numGlobalParts, scalar_t **pqJagged_partSizes){

	std::cout << "numLocalCoords:" << numLocalCoords << std::endl;
	std::cout << "coordDim:" << coordDim << std::endl;
	for(int i = 0; i < numLocalCoords; ++i){
		for (int ii = 0; ii < coordDim; ++ii){
			std::cout <<  pqJagged_values[ii][i] << " ";
		}
		std::cout << std::endl;
	}


	std::cout << "criteriaDim:" << criteriaDim << std::endl;
	std::cout << "weightDim:" << weightDim << std::endl;
	if(weightDim){
		for(int i = 0; i < numLocalCoords; ++i){
			for (int ii = 0; ii < weightDim; ++ii){
				std::cout <<  pqJagged_weights[ii][i] << " ";
			}
			std::cout << std::endl;
		}
	}

	std::cout << "pqJagged_uniformWeights:" << pqJagged_uniformWeights[0] << std::endl;
	for(int i = 0; i < criteriaDim; ++i){
		std::cout << pqJagged_uniformWeights[i] << " ";
	}
	std::cout << std::endl;


	std::cout << "gnos" << std::endl;
	for(int i = 0; i < numLocalCoords; ++i){
			std::cout <<  pqJagged_gnos[i] << " ";
	}
	std::cout << std::endl;

	std::cout << "ignoreWeights:" << ignoreWeights << std::endl;

	std::cout << "pqJagged_uniformParts:" << pqJagged_uniformParts[0] << std::endl;
	for(int i = 0; i < criteriaDim; ++i){
		std::cout << pqJagged_uniformParts[i] << " ";
	}
	std::cout << std::endl;

	std::cout << "pqJagged_partSizes:" << std::endl;
	std::cout << "numGlobalParts:" << numGlobalParts << std::endl;
	for(int i = 0; i < criteriaDim; ++i){
		if(!pqJagged_uniformParts[i])
		for(int ii = 0; ii < numGlobalParts; ++ii){
			std::cout << pqJagged_partSizes[i][ii] << " ";
		}
		std::cout << std::endl;
	}
}


int pqJagged_getNumThreads(){
	int numThreads;
#pragma omp parallel shared(numThreads)
	{
		numThreads = omp_get_num_threads();
	}

	return numThreads;
}

template <typename scalar_t>
void pqJagged_getMinMaxCoord(scalar_t *pqJagged_coordinates, scalar_t &minCoordinate, scalar_t &maxCoordinate,
		size_t numLocalCoords, const RCP<const Environment> &env, int numThreads,
		bool firstIteration, lno_t *partitionedPointCoordinates, lno_t coordinateBegin, lno_t coordinateEnd){

	int nt2 = numThreads * 2;
	scalar_t *max_min = new scalar_t[nt2];

#pragma omp parallel
	{
		int myId = omp_get_thread_num();
		scalar_t myMin, myMax;

		size_t j = firstIteration ? 0 : partitionedPointCoordinates[coordinateBegin];
		myMin = myMax = pqJagged_coordinates[j];
#pragma omp for
		for(size_t j = coordinateBegin + 1; j < coordinateEnd; ++j){
			int i = firstIteration ? j:partitionedPointCoordinates[coordinateBegin];
			if(pqJagged_coordinates[i] > myMax) myMax = pqJagged_coordinates[i];
			if(pqJagged_coordinates[i] < myMin) myMin = pqJagged_coordinates[i];
		}
		max_min[myId] = myMin;
		max_min[myId + numThreads] = myMax;
#pragma omp barrier


#pragma omp single nowait
		{
			minCoordinate = max_min[0];
			for(int i = 1; i < numThreads; ++i){
				if(max_min[i] < minCoordinate) minCoordinate = max_min[i];
			}
		}
#pragma omp single nowait
		{
			maxCoordinate = max_min[numThreads];
			for(int i = numThreads + 1; i < nt2; ++i){
				if(max_min[i] > maxCoordinate) maxCoordinate = max_min[i];
			}
		}

	}

	delete [] max_min;
}


template <typename scalar_t>
void pqJagged_getCutCoord_Weights(scalar_t minCoordinate, scalar_t maxCoordinate,
		bool pqJagged_uniformParts, scalar_t *pqJagged_partSizes /*p sized, weight ratios of each part*/ , size_t noCuts/*p-1*/ ,
		scalar_t *cutCoordinates /*p - 1 sized, coordinate of each cut line*/, scalar_t *cutPartRatios /*cumulative weight ratios, at left side of each cut line. p-1 sized*/){

	scalar_t coordinateRange = maxCoordinate - minCoordinate;
	if(pqJagged_uniformParts){
		scalar_t uniform = 1. / (noCuts + 1);
		scalar_t slice = uniform * coordinateRange;
#pragma omp parallel for
			for(size_t i = 0; i < noCuts; ++i){
				cutPartRatios[i] =  uniform * (i + 1);
				cutCoordinates[i] = slice * (i + 1);
			}
	}
	else {
		cutPartRatios[0] = pqJagged_partSizes[0];
		cutCoordinates[0] = coordinateRange * cutPartRatios[0];
		for(size_t i = 1; i < noCuts; ++i){
			cutPartRatios[i] = pqJagged_partSizes[i] + cutPartRatios[i - 1];
			cutCoordinates[i] = coordinateRange * cutPartRatios[i];
		}
	}
}

void getNewCoordinates(const size_t &total_part_count, const scalar_t * totalPartWeights, bool *isDone, const scalar_t *cutPartRatios,
		const scalar_t &totalWeight, const scalar_t &imbalanceTolerance, size_t &allDone, scalar_t *cutUpperBounds, scalar_t *cutLowerBounds,
		scalar_t *cutCoordinates, const size_t &noCuts, const scalar_t &maxCoordinate, const scalar_t &minCoordinate){
	scalar_t seenW = 0;
	float expected = 0;
	scalar_t leftImbalance = 0, rightImbalance = 0;

#pragma omp for
	for (size_t i = 0; i < total_part_count; i += 2){
		seenW = totalPartWeights[i];
		int currentLine = i/2;
		if(isDone[currentLine]) continue;
		expected = cutPartRatios[i / 2];

		leftImbalance = imbalanceOf(seenW, totalWeight, expected);
		rightImbalance = imbalanceOf(totalWeight - seenW, totalWeight, 1 - expected);

		//cout << "cut:" << currentLine<< " left and right:" << leftImbalance << " " << rightImbalance << endl;
		bool isLeftValid = leftImbalance <= imbalanceTolerance && leftImbalance >= -imbalanceTolerance;
		bool isRightValid = rightImbalance <= imbalanceTolerance && rightImbalance >= -imbalanceTolerance;

		if(isLeftValid && isRightValid){
			isDone[currentLine] = true;
			__sync_fetch_and_sub(&allDone, 1);
			//cout << "\tboth valid" << endl;
		} else if(cutUpperBounds[currentLine] != -1 && cutUpperBounds[currentLine] == cutLowerBounds[currentLine]){
			isDone[currentLine] = true;
			__sync_fetch_and_sub(&allDone, 1);
			//cout << "\tconverged upper:" <<  cutUpperBounds[currentLine] << " loweR:" << cutLowerBounds[currentLine] << endl;

		} else if(leftImbalance < 0){
			//TODO if boundary is enough binary search on processors
			//if boundary is not enough, binary search
			//cout << "moving to right" << endl;
			if (cutUpperBounds[currentLine] == -1){
				bool done = false;
				for (size_t ii = currentLine + 1; ii < noCuts ; ++ii){
					scalar_t pw = totalPartWeights[ii * 2];
					scalar_t ew = totalWeight * expected;
					if(pw >= ew){
						if(pw == ew){
							cutCoordinates[currentLine] = cutCoordinates[ii];
							isDone[currentLine] = true;
							__sync_fetch_and_sub(&allDone, 1);
						} else {
							cutUpperBounds[currentLine] = cutCoordinates[ii];
							cutLowerBounds[currentLine] = cutCoordinates [ ii -1];
							cutCoordinates[currentLine] = (cutLowerBounds[currentLine] + cutUpperBounds[currentLine]) / 2;
						}
						done = true;
						break;
					}
				}
				if(!done){
					cutUpperBounds[currentLine] = maxCoordinate;
					cutLowerBounds[currentLine] = cutCoordinates [ noCuts -1];
					cutCoordinates[currentLine] = (cutLowerBounds[currentLine] + cutUpperBounds[currentLine]) / 2;
				}

			} else {
				cutLowerBounds[currentLine] = cutCoordinates[currentLine];
				cutCoordinates[currentLine] = (cutLowerBounds[currentLine] + cutUpperBounds[currentLine]) / 2;
			}

		} else {
			//move left
			//new coordinate binary search
			if (cutLowerBounds[currentLine] == -1){

				bool done = false;
				for (int ii = currentLine - 1; ii >= 0; --ii){
					scalar_t pw = totalPartWeights[ii * 2];
					scalar_t ew = totalWeight * expected;
					if(pw <= ew){
						if(pw == ew){
							cutCoordinates[currentLine] = cutCoordinates[ii];
							isDone[currentLine] = true;
							__sync_fetch_and_sub(&allDone, 1);
						} else {
							cutLowerBounds[currentLine] = cutCoordinates[ii];
							cutUpperBounds[currentLine] = cutCoordinates[ii + 1];
							cutCoordinates[currentLine] = (cutLowerBounds[currentLine] + cutUpperBounds[currentLine]) / 2;
						}
						done = true;
						break;
					}

				}
				if(!done){
					cutUpperBounds[currentLine] = cutCoordinates[0];
					cutLowerBounds[currentLine] = minCoordinate;
					cutCoordinates[currentLine] = (cutLowerBounds[currentLine] + cutUpperBounds[currentLine]) / 2;

				}
			} else {
				cutUpperBounds[currentLine] = cutCoordinates[currentLine];
				cutCoordinates[currentLine] = (cutLowerBounds[currentLine] + cutUpperBounds[currentLine]) / 2;
			}

		}

	}
}


template <typename scalar_t, typename lno_t>
void pqJagged_1DPart(scalar_t *pqJagged_coordinates,	scalar_t *pqJagged_weights,	bool pqJagged_uniformWeights,
		const size_t &numLocalCoords,const global_size_t &numGlobalCoords,	scalar_t &minCoordinate,
		scalar_t &maxCoordinate, bool pqJagged_uniformParts, scalar_t *pqJagged_partSizes, size_t partNo, int noThreads,
		scalar_t imbalanceTolerance){

	imbalanceTolerance = 0.001;

	size_t noCuts = partNo - 1;
	scalar_t *cutCoordinates = new scalar_t[noCuts]; // coordinates of the cut lines. First one is the min, last one is max coordinate.
	scalar_t *cutPartRatios = new scalar_t[noCuts]; // the weight ratios at left side of the cuts. First is 0, last is 1.

	scalar_t *cutUpperBounds = new scalar_t [noCuts];  //to determine the next cut line with binary search
	scalar_t *cutLowerBounds = new scalar_t [noCuts];  //to determine the next cut line with binary search

	pqJagged_getCutCoord_Weights<scalar_t>(minCoordinate, maxCoordinate,
			pqJagged_uniformParts, pqJagged_partSizes, noCuts,
			cutCoordinates, cutPartRatios);

	//make a function for this.
	//assuming that multiplication via 0 or 1 is faster than branches.
	scalar_t *wghts = NULL;
	scalar_t unit_weight = 1;
	lno_t v_scaler;
	if (pqJagged_uniformWeights){
		wghts = &unit_weight;
		v_scaler = 0;
	}
	else {
		wghts = pqJagged_weights;
		v_scaler = 1;
	}


	//calculate total weight
	scalar_t totalWeight = 0;

	lno_t *coordinate_linked_list = new lno_t[numLocalCoords];
	lno_t **coordinate_starts = new lno_t *[noThreads];
	lno_t **coordinate_ends = new lno_t *[noThreads];
	scalar_t **partWeights = new scalar_t *[noThreads];

	bool *isDone = new bool [noCuts];

	size_t total_part_count = partNo + noCuts;
	for(int i = 0; i < noThreads; ++i){
		coordinate_starts[i] = new lno_t[total_part_count];
		coordinate_ends[i] = new lno_t[total_part_count];
		partWeights[i] = new scalar_t[total_part_count];
	}

	scalar_t *totalPartWeights = new scalar_t[total_part_count];

	size_t allDone = noCuts;
#pragma omp parallel shared(allDone)
	{
#pragma omp for reduction(+:totalWeight)
		for (size_t i = 0; i < numLocalCoords; ++i){
			totalWeight += wghts[i * v_scaler];
		}
		//TODO: mpi all reduce to obtain total weight.

		int me = omp_get_thread_num();
		lno_t *myStarts = coordinate_starts[me];
		lno_t *myEnds = coordinate_ends[me];
		scalar_t *myPartWeights = partWeights[me];


		pqJagged_PartVertices <lno_t, size_t> pqPV;
		pqPV.set(numLocalCoords, coordinate_linked_list, myStarts, myEnds);

#pragma omp for
		for(size_t i = 0; i < noCuts; ++i){
			isDone[i] = false;
			cutLowerBounds[i] = -1;
			cutUpperBounds[i] = -1;
		}

#pragma omp for
		for (size_t i = 0; i < numLocalCoords; ++i){
			coordinate_linked_list[i] = -1;
		}
		//implicit barrier


		while (allDone != 0){
			cout << "allDone:" << allDone << endl;
/*
			for (size_t i = 0; i < noCuts; ++i){
				cout << "i:" << i << " coordinate:" << cutCoordinates[i] << endl;
			}
*/

			for (size_t i = 0; i < total_part_count; ++i){
				myEnds[i] = -1;
				myStarts[i] = -1;
				myPartWeights[i] = 0;
			}

#pragma omp for
			for (size_t i = 0; i < numLocalCoords; ++i){
				for(size_t j = 0; j < noCuts; ++j){
					if (pqJagged_coordinates[i] <= cutCoordinates[j]){
						if (pqJagged_coordinates[i] == cutCoordinates[j]){
							myPartWeights[j * 2] -=	wghts [i * v_scaler];
							pqPV.inserToPart(j * 2 + 1, i);
						}
						else {
							pqPV.inserToPart(j * 2, i);
						}
						break;
					} else {
						scalar_t w = wghts [i * v_scaler];
						myPartWeights[j * 2] -=	 w ;
						myPartWeights[j * 2 + 1] -=	w;
					}
				}
			}


#pragma omp for
			for(size_t j = 0; j < total_part_count; ++j){
				scalar_t pwj = 0;
				for (int i = 0; i < noThreads; ++i){
					pwj += partWeights[i][j];
				}
				totalPartWeights[j] = pwj + totalWeight;
			}

			//all to all partweight send;
			//get totalpartweights from different nodes sized total_part_count

getNewCoordinates(total_part_count, totalPartWeights, isDone, cutPartRatios,
					totalWeight, imbalanceTolerance, allDone, cutUpperBounds, cutLowerBounds,
					cutCoordinates, noCuts,maxCoordinate, minCoordinate);
		}
	}

	for(size_t j = 0; j < total_part_count; j+=2){
		cout << "j:" << j/2 << " pw:" << totalPartWeights[j] << endl;
	}
}





void getNewCoordinates_simple(const size_t &total_part_count, const scalar_t * totalPartWeights, bool *isDone, const scalar_t *cutPartRatios,
		const scalar_t &totalWeight, const scalar_t &imbalanceTolerance, size_t &allDone, scalar_t *cutUpperBounds, scalar_t *cutLowerBounds,
		scalar_t *&cutCoordinates, const size_t &noCuts, const scalar_t &maxCoordinate, const scalar_t &minCoordinate,
		scalar_t *leftClosestDistance, scalar_t *rightClosestDistance, scalar_t * cutLowerWeight,scalar_t * cutUpperWeight, scalar_t *&cutCoordinatesWork){
	scalar_t seenW = 0;
	float expected = 0;
	scalar_t leftImbalance = 0, rightImbalance = 0;

	scalar_t _EPSILON = numeric_limits<scalar_t>::epsilon();

#pragma omp for
	for (size_t i = 0; i < noCuts; i++){
		if(leftClosestDistance[i] < 0) leftClosestDistance[i] = 0;
		if(rightClosestDistance[i] < 0) rightClosestDistance[i] = 0;
		seenW = totalPartWeights[i * 2];
		int currentLine = i;
		if(isDone[currentLine]) {
			cutCoordinatesWork[currentLine] = cutCoordinates[currentLine];
			continue;
		}
		expected = cutPartRatios[i];

		leftImbalance = imbalanceOf(seenW, totalWeight, expected);
		rightImbalance = imbalanceOf(totalWeight - seenW, totalWeight, 1 - expected);


		bool isLeftValid = leftImbalance <= imbalanceTolerance && leftImbalance >= -imbalanceTolerance;
		bool isRightValid = rightImbalance <= imbalanceTolerance && rightImbalance >= -imbalanceTolerance;
/*
		if(i == 83 || i == 84){
		cout << "up:" << cutUpperBounds[currentLine] << " low:" << cutLowerBounds[currentLine]<< " fark:" << cutUpperBounds[currentLine] - cutLowerBounds[currentLine] <<  "eps:" << _EPSILON<<endl;
		cout << "cut:" << cutCoordinates[currentLine]<< " left and right-:" << leftImbalance << " " << rightImbalance << endl;
		}
		*/


		if(isLeftValid && isRightValid){
			isDone[currentLine] = true;
			__sync_fetch_and_sub(&allDone, 1);
			cutCoordinatesWork [ currentLine] = cutCoordinates[currentLine];
		} else if(cutUpperBounds[currentLine] != -1 && (cutUpperBounds[currentLine] - cutLowerBounds[currentLine]) < _EPSILON ){
			isDone[currentLine] = true;
			__sync_fetch_and_sub(&allDone, 1);
			cutCoordinatesWork [ currentLine] = cutCoordinates[currentLine];
		} else if(leftImbalance < 0){

			scalar_t ew = totalWeight * expected;

			//TODO if boundary is enough binary search on processors
			//if boundary is not enough, binary search
			//cout << "moving to right" << endl;
			if (cutUpperBounds[currentLine] == -1){
				bool done = false;
				for (size_t ii = currentLine + 1; ii < noCuts ; ++ii){
					scalar_t pw = totalPartWeights[ii * 2];
/*
					if(i == 83){
						cout << "pw:" << pw << " ew:" << ew << endl;
					}
					*/
					if(pw >= ew){
						if(pw == ew){
							isDone[currentLine] = true;
							__sync_fetch_and_sub(&allDone, 1);
							cutCoordinatesWork [ currentLine] = cutCoordinates[ii];
						} else {
							cutUpperBounds[currentLine] = cutCoordinates[ii] - leftClosestDistance[ii] + _EPSILON;
							cutUpperWeight[currentLine] = totalPartWeights [2 * ii + 1];

							cutLowerBounds[currentLine] = cutCoordinates [ ii -1] + rightClosestDistance[ii - 1] - _EPSILON;
							cutLowerWeight[currentLine] = totalPartWeights [2 * ii - 1];


							scalar_t newPivot = pivotPos<scalar_t> (cutUpperBounds, cutLowerBounds,currentLine, cutUpperWeight, cutLowerWeight, ew);
							if (fabs(cutCoordinates[currentLine] - newPivot) < _EPSILON){
								isDone[currentLine] = true;
								__sync_fetch_and_sub(&allDone, 1);
								cutCoordinatesWork [ currentLine] = cutCoordinates[currentLine];
							} else {
								cutCoordinatesWork [ currentLine] = newPivot;
							}

						}

/*
						if(i == 83){
						cout << "up:" << cutUpperBounds[currentLine] << " low:" << cutLowerBounds[currentLine]<< endl;
						cout << "ii:" << ii << endl;
						}
*/
						done = true;
						break;
					}
				}
				if(!done){
					cutUpperBounds[currentLine] = maxCoordinate;
					cutUpperWeight[currentLine] = totalWeight;

					cutLowerBounds[currentLine] = cutCoordinates [ noCuts -1] + rightClosestDistance[noCuts - 1] - _EPSILON;
					cutLowerWeight[currentLine] = totalPartWeights [2 * noCuts - 1];

					scalar_t newPivot = pivotPos<scalar_t> (cutUpperBounds, cutLowerBounds,currentLine, cutUpperWeight, cutLowerWeight, ew);
					if (fabs(cutCoordinates[currentLine] - newPivot) < _EPSILON){
						isDone[currentLine] = true;
						__sync_fetch_and_sub(&allDone, 1);
						cutCoordinatesWork [ currentLine] = cutCoordinates[currentLine];
					} else {
						cutCoordinatesWork [ currentLine] = newPivot;
					}
				}

			} else {


				cutLowerBounds[currentLine] = cutCoordinates[currentLine] + rightClosestDistance[currentLine] - _EPSILON;
				cutLowerWeight[currentLine] = seenW;

				for (size_t ii = currentLine + 1; ii < noCuts ; ++ii){
					scalar_t pw = totalPartWeights[ii * 2];

					if(pw >= ew){
						if(pw == ew){

							cutCoordinatesWork[currentLine] = cutCoordinates[ii];
							isDone[currentLine] = true;
							__sync_fetch_and_sub(&allDone, 1);
						} else if (pw < cutUpperWeight[currentLine]){
							cutUpperBounds[currentLine] = cutCoordinates[ii] - leftClosestDistance[ii] + _EPSILON;
							cutUpperWeight[currentLine] = totalPartWeights [2 * ii + 1];
						}
						break;
					}
					if (pw <= ew && pw >= cutLowerWeight[currentLine]){
						cutLowerBounds[currentLine] = cutCoordinates[ii] + rightClosestDistance[ii] - _EPSILON;
						cutLowerWeight[currentLine] = pw;
					}

				}
				scalar_t newPivot = pivotPos<scalar_t> (cutUpperBounds, cutLowerBounds,currentLine, cutUpperWeight, cutLowerWeight, ew);
				if (fabs(cutCoordinates[currentLine] - newPivot) < _EPSILON){
					isDone[currentLine] = true;
					__sync_fetch_and_sub(&allDone, 1);
					cutCoordinatesWork [ currentLine] = cutCoordinates[currentLine];
				} else {
					cutCoordinatesWork [ currentLine] = newPivot;
				}
			}

		} else {

			scalar_t ew = totalWeight * expected;
			if (cutLowerBounds[currentLine] == -1){

				bool done = false;
				for (int ii = currentLine - 1; ii >= 0; --ii){
					scalar_t pw = totalPartWeights[ii * 2];
					if(pw <= ew){
						if(pw == ew){
							cutCoordinatesWork[currentLine] = cutCoordinates[ii];
							isDone[currentLine] = true;
							__sync_fetch_and_sub(&allDone, 1);
						} else {


							cutUpperBounds[currentLine] = cutCoordinates[ii + 1] - leftClosestDistance[ii + 1] + _EPSILON;
							cutUpperWeight[currentLine] = totalPartWeights [2 * ii + 3];


							cutLowerBounds[currentLine] = cutCoordinates[ii] + rightClosestDistance[ii] - _EPSILON;
							cutLowerWeight[currentLine] = totalPartWeights [2 * ii + 1];

							scalar_t newPivot = pivotPos<scalar_t> (cutUpperBounds, cutLowerBounds,currentLine, cutUpperWeight, cutLowerWeight, ew);
							if (fabs(cutCoordinates[currentLine] - newPivot) < _EPSILON){
								isDone[currentLine] = true;
								__sync_fetch_and_sub(&allDone, 1);
								cutCoordinatesWork [ currentLine] = cutCoordinates[currentLine];
							} else {
								cutCoordinatesWork [ currentLine] = newPivot;
							}

						}
						done = true;
						break;
					}

				}
				if(!done){

					cutUpperBounds[currentLine] = cutCoordinates[0] - leftClosestDistance[0] + _EPSILON;
					cutLowerBounds[currentLine] = minCoordinate;

					cutUpperWeight[currentLine] = totalPartWeights [1];
					cutLowerWeight[currentLine] = 0;

					scalar_t newPivot = pivotPos<scalar_t> (cutUpperBounds, cutLowerBounds,currentLine, cutUpperWeight, cutLowerWeight, ew);
					if (fabs(cutCoordinates[currentLine] - newPivot) < _EPSILON){
						isDone[currentLine] = true;
						__sync_fetch_and_sub(&allDone, 1);
						cutCoordinatesWork [ currentLine] = cutCoordinates[currentLine];
					} else {
						cutCoordinatesWork [ currentLine] = newPivot;
					}

				}
			} else {
				cutUpperBounds[currentLine] = cutCoordinates[currentLine] - leftClosestDistance[currentLine] + _EPSILON;
				cutUpperWeight[currentLine] = seenW;

				for (int ii = currentLine - 1; ii >= 0; --ii){
					scalar_t pw = totalPartWeights[ii * 2];

					if(pw <= ew){
						if(pw == ew){
							cutCoordinatesWork[currentLine] = cutCoordinates[ii];
							isDone[currentLine] = true;
							__sync_fetch_and_sub(&allDone, 1);
						} else if (pw > cutLowerWeight[currentLine]){
							cutLowerBounds[currentLine] = cutCoordinates[ii] + rightClosestDistance[ii] - _EPSILON;
							cutLowerWeight[currentLine] = totalPartWeights [2 * ii + 1];
						}
						break;
					}
					if (pw >= ew && pw < cutUpperWeight[currentLine]){
						cutUpperBounds[currentLine] = cutCoordinates[ii] - leftClosestDistance[ii] + _EPSILON;
						cutUpperWeight[currentLine] = pw;
					}

				}

				scalar_t newPivot = pivotPos<scalar_t> (cutUpperBounds, cutLowerBounds,currentLine, cutUpperWeight, cutLowerWeight, ew);
				if (fabs(cutCoordinates[currentLine] - newPivot) < _EPSILON){
					isDone[currentLine] = true;
					__sync_fetch_and_sub(&allDone, 1);
					cutCoordinatesWork [ currentLine] = cutCoordinates[currentLine];
				} else {
					cutCoordinatesWork [ currentLine] = newPivot;
				}
			}

		}
	}

#pragma omp barrier
#pragma omp single
	{
	scalar_t *t = cutCoordinatesWork;
	cutCoordinatesWork = cutCoordinates;
	cutCoordinates = t;
	}
}


template <typename scalar_t, typename lno_t>
void pqJagged_1DPart_simple(scalar_t *pqJagged_coordinates,	scalar_t *pqJagged_weights,	bool pqJagged_uniformWeights,
		const size_t &numLocalCoords,const global_size_t &numGlobalCoords,	scalar_t &minCoordinate,
		scalar_t &maxCoordinate, bool pqJagged_uniformParts, scalar_t *pqJagged_partSizes, size_t partNo, int noThreads,
		scalar_t imbalanceTolerance, scalar_t *cutCoordinates,
		bool firstIteration, lno_t *partitionedPointCoordinates, lno_t coordinateBegin, lno_t coordinateEnd){

	scalar_t *cutCoordinates_tmp = cutCoordinates;


	size_t noCuts = partNo - 1;
	scalar_t *cutCoordinatesWork = new scalar_t [noCuts]; // work array to manipulate coordinate of cutlines in different iterations.
	scalar_t *cutPartRatios = new scalar_t[noCuts]; // the weight ratios at left side of the cuts. First is 0, last is 1.

	scalar_t *cutUpperBounds = new scalar_t [noCuts];  //to determine the next cut line with binary search
	scalar_t *cutLowerBounds = new scalar_t [noCuts];  //to determine the next cut line with binary search

	scalar_t *cutLowerWeight = new scalar_t [noCuts];  //to determine the next cut line with binary search
	scalar_t *cutUpperWeight = new scalar_t [noCuts];  //to determine the next cut line with binary search


	pqJagged_getCutCoord_Weights<scalar_t>(minCoordinate, maxCoordinate,
			pqJagged_uniformParts, pqJagged_partSizes, noCuts,
			cutCoordinates_tmp, cutPartRatios);

	//make a function for this.
	//assuming that multiplication via 0 or 1 is faster than branches.
	/*
	scalar_t *wghts = NULL;
	scalar_t unit_weight = 1;
	lno_t v_scaler;
	if (pqJagged_uniformWeights){
		wghts = &unit_weight;
		v_scaler = 0;
	}
	else {
		wghts = pqJagged_weights;
		v_scaler = 1;
	}
	*/


	//calculate total weight
	scalar_t totalWeight = 0;

	bool *isDone = new bool [noCuts];
	scalar_t **partWeights = new scalar_t *[noThreads];

	scalar_t **leftClosestDistance = new scalar_t* [noThreads];
	scalar_t **rightClosestDistance = new scalar_t* [noThreads];

	size_t total_part_count = partNo + noCuts;
	for(int i = 0; i < noThreads; ++i){
		partWeights[i] = new scalar_t[total_part_count];
		rightClosestDistance[i] = new scalar_t [noCuts];
		leftClosestDistance[i] = new scalar_t [noCuts];
	}

	scalar_t *totalPartWeights = new scalar_t[total_part_count];

	size_t allDone = noCuts;


#pragma omp parallel shared(allDone)
	{
		int iterationCount = 0;

		//get total weight.
		//TODO:check if there is a pre-calculated value.
		//or
		//TODO: mpi all reduce to obtain total weight.
		if (pqJagged_uniformWeights) {
			totalWeight = coordinateEnd - coordinateBegin;
		} else {

#pragma omp for reduction(+:totalWeight)
			for (size_t ii = coordinateBegin; ii < coordinateEnd; ++ii){
				int i = firstIteration? ii:partitionedPointCoordinates[ii];
				scalar_t w = pqJagged_uniformWeights ? 1:pqJagged_weights[i];
				totalWeight += w;
			}
		}
		scalar_t globaltotalWeight = totalWeight; // reduce totalweight here. TODO

		int me = omp_get_thread_num();
		scalar_t *myPartWeights = partWeights[me];
		scalar_t *myLeftClosest = leftClosestDistance[me];
		scalar_t *myRightClosest = rightClosestDistance[me];


		//pqJagged_PartVertices <lno_t, size_t> pqPV;
		//pqPV.set(numLocalCoords, coordinate_linked_list, myStarts, myEnds);


#pragma omp for
		for(size_t i = 0; i < noCuts; ++i){
			isDone[i] = false;
			cutLowerBounds[i] = -1;
			cutUpperBounds[i] = -1;

		}


		while (allDone != 0){
			iterationCount++;
/*
#pragma omp single
			{
				if(iterationCount != 1) {
				cout << "alldone:" << allDone << endl;
				for (size_t i = 0; i < noCuts; ++i){
					if (!isDone[i])
					{cout << "iter:" << iterationCount<< " i:" << i<< " is not done! " << cutCoordinates[i] << " " << cutUpperBounds[i] << " "<< cutLowerBounds[i] << endl;
					}
				}
				}
			}
*/
			/*
#pragma omp single
			{
				int i = 83;
				if(iterationCount > 1 && !isDone[i])
				cout << "iter:" << iterationCount<< " i:" << i<< " is not done! " << cutCoordinates[i] << " " << cutUpperBounds[i] << " "<< cutLowerBounds[i] << endl;
				else
					cout << "iter:" << iterationCount<< " i:" << i<< " is done! " << cutCoordinates[i] << " " << cutUpperBounds[i] << " "<< cutLowerBounds[i] << endl;
				i = 84;
				if(iterationCount > 1 && !isDone[i])
								cout << "iter:" << iterationCount<< " i:" << i<< " is not done! " << cutCoordinates[i] << " " << cutUpperBounds[i] << " "<< cutLowerBounds[i] << endl;
				else
					cout << "iter:" << iterationCount<< " i:" << i<< " is done! " << cutCoordinates[i] << " " << cutUpperBounds[i] << " "<< cutLowerBounds[i] << endl;
			}
			*/
			for (size_t i = 0; i < total_part_count; ++i){
				myPartWeights[i] = 0;
			}
			for(size_t i = 0; i < noCuts; ++i){
				myLeftClosest[i] = -1;
				myRightClosest[i] = -1;
			}

#pragma omp for
			for (size_t ii = coordinateBegin; ii < coordinateEnd; ++ii){
				int i = firstIteration? ii: partitionedPointCoordinates[ii];

				for(size_t j = 0; j < noCuts; ++j){
					scalar_t distance = pqJagged_coordinates[i] - cutCoordinates_tmp[j];
					if (distance < 0) {
						distance = -distance;
						if (myLeftClosest[j] < 0 || myLeftClosest[j] > distance){
							myLeftClosest[j] = distance;
						}
						break;
					}
					else if (distance == 0){
						scalar_t w = pqJagged_uniformWeights? 1:pqJagged_weights[i];
						myPartWeights[j * 2] -=	w;
						myLeftClosest[j] = 0;
						myRightClosest[j] = 0;
						break;
					} else {
						scalar_t w = pqJagged_uniformWeights? 1:pqJagged_weights[i];
						myPartWeights[j * 2] -=	w;
						myPartWeights[j * 2 + 1] -=	w;
						if (myRightClosest[j] < 0 || myRightClosest[j] > distance){
							myRightClosest[j] = distance;
						}
					}
				}
			}

#pragma omp for
			for(size_t i = 0; i < noCuts; ++i){
				scalar_t minl = leftClosestDistance[0][i], minr = rightClosestDistance[0][i];

				for (int j = 1; j < noThreads; ++j){
					if ((rightClosestDistance[j][i] < minr || minr < 0) && rightClosestDistance[j][i] >= 0 ){
						minr = rightClosestDistance[j][i];
					}
					if ((leftClosestDistance[j][i] < minl || minl < 0) && leftClosestDistance[j][i] >= 0){
						minl = leftClosestDistance[j][i];
					}
				}
				leftClosestDistance[0][i] = minl;
				rightClosestDistance[0][i] = minr;
			}

#pragma omp for
			for(size_t j = 0; j < total_part_count; ++j){
				scalar_t pwj = 0;
				for (int i = 0; i < noThreads; ++i){
					pwj += partWeights[i][j];
				}
				totalPartWeights[j] = pwj + totalWeight;
			}


			//all to all partweight send;
			//get totalpartweights from different nodes sized total_part_count

/*
#pragma omp single
			{

				cout << "left:";
				for(size_t j = 0; j < noCuts; ++j){
					cout <<  leftClosestDistance[0][j] << " " ; //" left:" << myLeftClosest[j] << " right:" << myRightClosest[j] << " ";
				}
				cout << endl;

				cout << "right:";
				for(size_t j = 0; j < noCuts; ++j){
					cout <<  rightClosestDistance[0][j] << " " ; //" left:" << myLeftClosest[j] << " right:" << myRightClosest[j] << " ";
				}
				cout << endl;


				cout << "current:";
				for(size_t j = 0; j < noCuts; ++j){
					cout << cutCoordinates[j] << " " ; //" left:" << myLeftClosest[j] << " right:" << myRightClosest[j] << " ";
				}
				cout << endl;


			}
*/


			/*
#pragma omp single
			{
				for(size_t j = 0; j < noCuts; ++j){
					cout << leftClosestDistance[0][j] << " ";
				}
				cout << endl;

				for(size_t j = 0; j < noCuts; ++j){
					cout << rightClosestDistance[0][j] << " ";
				}
				cout << endl;
			}
*/
			//reduce part sizes here within all mpi processes. under totalPartweights TODO

			getNewCoordinates_simple(total_part_count, totalPartWeights, isDone, cutPartRatios,
					globaltotalWeight, imbalanceTolerance, allDone, cutUpperBounds, cutLowerBounds,
					cutCoordinates_tmp, noCuts,maxCoordinate, minCoordinate, leftClosestDistance[0],
					rightClosestDistance[0],cutLowerWeight, cutUpperWeight,cutCoordinatesWork);
		}

#pragma omp single
		{
		cout << "cutCoordinates:" << cutCoordinates << " cutCoordinates_tmp:" << cutCoordinates_tmp << endl;
		}

		if (cutCoordinates != cutCoordinates_tmp){
#pragma omp for
			for(size_t i = 0; i < noCuts; ++i){
				cutCoordinates[i] = cutCoordinates_tmp[i];
			}
#pragma omp single
			{
				cutCoordinatesWork = cutCoordinates_tmp;
			}

		}

#pragma omp single
		{
			cout << "Iteration:" << iterationCount << endl;
		}
	}


	float maxi = imbalanceOf(totalPartWeights[0], totalWeight, cutPartRatios[0]);

	float mini = imbalanceOf(totalPartWeights[0], totalWeight, cutPartRatios[0]);
	for(size_t j = 0; j < total_part_count - 1; j+=2){
		float imb = imbalanceOf(totalPartWeights[j], totalWeight, cutPartRatios[j/2]);
		cout << "j:" << j/2 << " pw:" << totalPartWeights[j] << " with imbalance:" << imb << endl;
		if(imb > maxi) maxi = imb;
		if(imb < mini) mini = imb;

	}

	cout << "max imbalance:" << maxi << " min imbalance:" << mini << endl;
	for(int i = 0; i < noCuts - 1; ++i){
		if(cutCoordinates_tmp[i] >= cutCoordinates_tmp[i+1]){
			cout << "ERROR:" << " cutCoordinates[" << i<<"]:" << cutCoordinates_tmp[i] << endl;
		}
	}


	cout << minCoordinate << " " << maxCoordinate << endl;
	for(int i = 0; i < noCuts; ++i){
			cout << "cutCoordinates[" << i<<"]:"<< cutCoordinates_tmp[i]<< endl;
	}


	delete []cutCoordinatesWork; // work array to manipulate coordinate of cutlines in different iterations.
	delete []cutPartRatios; // the weight ratios at left side of the cuts. First is 0, last is 1.

	delete []cutUpperBounds;  //to determine the next cut line with binary search
	delete []cutLowerBounds;  //to determine the next cut line with binary search

	delete []cutLowerWeight;  //to determine the next cut line with binary search
	delete []cutUpperWeight;  //to determine the next cut line with binary search

	delete []isDone;
	delete []totalPartWeights;

	for(int i = 0; i < noThreads; ++i){
		delete [] partWeights[i] ;
		delete [] rightClosestDistance[i];
		delete [] leftClosestDistance[i];
	}

	delete [] partWeights;

	delete [] leftClosestDistance ;
	delete [] rightClosestDistance;




}


template <typename lno_t, typename scalar_t>
void getChunksFromCoordinates(size_t partNo, size_t numLocalCoords, int noThreads,
		scalar_t *pqJagged_coordinates, scalar_t *cutCoordinates, lno_t *totalCounts,
		lno_t *partitionedPointCoordinates,
		bool firstIteration, lno_t *newpartitionedPointCoordinates, lno_t coordinateBegin, lno_t coordinateEnd){

	lno_t *coordinate_linked_list = new lno_t[coordinateEnd]; // pass it as argument. TODO
	lno_t **coordinate_starts = new lno_t *[noThreads];
	lno_t **coordinate_ends = new lno_t *[noThreads];

	lno_t **partPointCounts = new lno_t *[noThreads];
	size_t noCuts = partNo - 1;

	for(int i = 0; i < noThreads; ++i){
		coordinate_starts[i] = new lno_t[partNo];
		coordinate_ends[i] = new lno_t[partNo];
		if(i == 0){
			partPointCounts[i] = totalCounts;
		} else {
			partPointCounts[i] = new lno_t[partNo];
		}
	}

#pragma omp parallel
	{

		int me = omp_get_thread_num();
		lno_t *myStarts = coordinate_starts[me];
		lno_t *myEnds = coordinate_ends[me];
		lno_t *myPartPointCounts = partPointCounts[me];


		pqJagged_PartVertices <lno_t, size_t> pqPV;
		pqPV.set(numLocalCoords, coordinate_linked_list, myStarts, myEnds);
		memset(myPartPointCounts, 0, sizeof(lno_t) * partNo);

#pragma omp for
		for (size_t i = 0; i < numLocalCoords; ++i){
			coordinate_linked_list[i] = -1;
		}

		for (size_t i = 0; i < partNo; ++i){
			myEnds[i] = -1;
			myStarts[i] = -1;
		}




#pragma omp for
		for (size_t ii = coordinateBegin; ii < coordinateEnd; ++ii){
			int i = firstIteration ? ii: partitionedPointCoordinates[ii];
			bool inserted = false;
			for(size_t j = 0; j < noCuts; ++j){
				if (pqJagged_coordinates[i] < cutCoordinates[j]){
					pqPV.inserToPart(j, i);
					inserted = true;
					break;
				} else {
					myPartPointCounts[j] -=	 1;
				}
			}
			if(!inserted){
				pqPV.inserToPart(noCuts, i);
			}
		}

/*
#pragma omp single
		for(size_t i = 0; i < partNo; ++i){

			for(int j = 0; j < noThreads; ++j){
				cout << "part:"<< i<< "start:" << coordinate_starts[j][i] << " end:" << coordinate_ends[j][i] << endl;
			}
		}
*/
#pragma omp for
		for(size_t i = 0; i < partNo; ++i){

			int head = 0, tail = 0;
			while(coordinate_ends[head][i] == -1){
				++head;
			}
			int firstHead = head;
			for(int j = head; j < noThreads; ){

				tail = j+1;
				bool foundTail = false;
				while(tail < noThreads){
					if(coordinate_starts[tail][i] == -1){
						++tail;
					}
					else {
						foundTail = true;
						break;
					}
				}
				if(foundTail){
					//cout << "head:" << head << " tail:" << tail << " coordinate_ends[head][i]:" << coordinate_ends[head][i] << " coordinate_starts[tail][i]:" << coordinate_starts[tail][i] << endl;
					coordinate_linked_list[coordinate_ends[head][i]] = coordinate_starts[tail][i];
					head = tail;
				}
				j = tail;

			}
			coordinate_starts[0][i] = coordinate_starts[firstHead][i];
		}

#pragma omp for
		for(size_t j = 0; j < partNo; ++j){
			scalar_t pwj = 0;
			for (int i = 0; i < noThreads; ++i){
				pwj += partPointCounts[i][j];
			}
			totalCounts[j] = pwj + numLocalCoords;
		}


#pragma omp for
		for(size_t i = 0; i < partNo; ++i){
			lno_t nextPoint = coordinate_starts[0][i];
			lno_t pcnt = 0;

			lno_t prevCount = 0;
			if (i > 0) prevCount = totalCounts[i -1];
			while(nextPoint != -1){
				//cout << nextPoint << " ";
				partitionedPointCoordinates[prevCount + pcnt++] = nextPoint;
				nextPoint = coordinate_linked_list[nextPoint];
			}
			//cout << endl;

			//cout << "i:" << i<< " pcnt:" << pcnt << " prevCount:" << prevCount << " totalCounts:" << totalCounts[i] << endl;
		}

	}



	delete [] coordinate_linked_list;

	for(int i = 0; i < noThreads; ++i){
		delete [] coordinate_starts[i];
		delete [] coordinate_ends[i];
		if(i != 0){
			delete [] partPointCounts[i];
		}
	}

	delete []coordinate_starts;
	delete []coordinate_ends;
	delete []partPointCounts;

}

template <typename Adapter>
void AlgPQJagged(
  const RCP<const Environment> &env,
  RCP<Comm<int> > &comm,
  const RCP<const CoordinateModel<
    typename Adapter::base_adapter_t> > &coords,
  RCP<PartitioningSolution<Adapter> > &solution
)
{
#ifdef RCBCODE
	// Make a copy for global ops at the end because
	// we subdivide the communicator during the algorithm.
	// TODO memory leak here.
	RCP<Comm<int> > problemComm = comm->duplicate();
#endif

	typedef typename Adapter::scalar_t scalar_t;
	typedef typename Adapter::gno_t gno_t;

	typedef typename Adapter::lno_t lno_t;
	const Teuchos::ParameterList &pl = env->getParameters();

	std::bitset<NUM_RCB_PARAMS> params;
	int numTestCuts = 5;
	scalar_t imbalanceTolerance;

	multiCriteriaNorm mcnorm;
	bool ignoreWeights;
	pqJagged_getParameters<scalar_t>(pl, imbalanceTolerance, mcnorm, params, numTestCuts, ignoreWeights);

#ifdef RCBCODE
	env->debug(DETAILED_STATUS, string("Entering AlgPartRCB"));
	env->timerStart(BOTH_TIMERS, "RCB set up");
#endif

	int coordDim, weightDim; size_t numLocalCoords; global_size_t numGlobalCoords; int criteriaDim;
	pqJagged_getCoordinateValues<Adapter>( coords, coordDim, weightDim, numLocalCoords, numGlobalCoords, criteriaDim, ignoreWeights);

	scalar_t **pqJagged_coordinates = new scalar_t *[coordDim];
	scalar_t **pqJagged_weights = new scalar_t *[criteriaDim];
	bool *pqJagged_uniformParts = new bool[criteriaDim];
	scalar_t **pqJagged_partSizes = new scalar_t*[criteriaDim];
	bool *pqJagged_uniformWeights = new bool[criteriaDim];

	gno_t *pqJagged_gnos; // do we really need this?

	size_t numGlobalParts;
	int pqJagged_multiVectorDim;



#ifdef RCBCODE
	//defining adapters
	typedef typename Adapter::node_t node_t;
	typedef typename Adapter::lno_t lno_t;
	typedef typename Adapter::gno_t gno_t;

	typedef Tpetra::Map<lno_t, gno_t, node_t> map_t;
	typedef Tpetra::MultiVector<scalar_t, lno_t, gno_t, node_t> mvector_t;
	RCP<mvector_t> mvector;
	typedef StridedData<lno_t, scalar_t> input_t;
	ArrayRCP<bool> uniformWeights(new bool [criteriaDim], 0, criteriaDim, true);
	Array<ArrayRCP<scalar_t> > partSizes(criteriaDim);
	int multiVectorDim;
#endif

	pqJagged_getInputValues<Adapter, scalar_t, gno_t>(
			env, coords, solution,params,coordDim,weightDim,numLocalCoords,
			numGlobalCoords, numGlobalParts, pqJagged_multiVectorDim,
			pqJagged_coordinates,criteriaDim, pqJagged_weights,pqJagged_gnos, ignoreWeights,
		    pqJagged_uniformWeights, pqJagged_uniformParts, pqJagged_partSizes
#ifdef RCBCODE
			,uniformWeights,
			mvector,
			multiVectorDim,
			partSizes,
			problemComm
#endif
	);

	/*
	pqJagged_printInput<scalar_t, gno_t>(coordDim, weightDim, numLocalCoords, numGlobalCoords,
			criteriaDim, pqJagged_coordinates, pqJagged_weights,
			pqJagged_uniformParts, pqJagged_uniformWeights, pqJagged_gnos,
			ignoreWeights,numGlobalParts, pqJagged_partSizes);
*/


	int numThreads = pqJagged_getNumThreads();

	cout << "numThreads=" << numThreads << endl;
	scalar_t minCoordinate, maxCoordinate;
	/*
	for (int i = 0; i < coordDim; ++i){
		pqJagged_getMinMaxCoord<scalar_t>(pqJagged_coordinates[i], minCoordinate,maxCoordinate, numLocalCoords, env, numThreads);
		//cout << "i:" << i << " m:" << minCoordinate << " max:" << maxCoordinate << endl;

	}
	*/


	size_t *partNo = new size_t[coordDim];
	size_t totalDimensionCut = 0;
	size_t totalPartCount = 1;
	for (int i = 0; i < coordDim; ++i){
		partNo[i] = numGlobalParts;
		totalPartCount *= partNo[i];
	}
	totalDimensionCut = totalPartCount - 1;
	cout << totalDimensionCut << endl;
	cout << totalPartCount << endl;



	scalar_t *allCutCoordinates = new scalar_t[totalDimensionCut]; // coordinates of the cut lines. First one is the min, last one is max coordinate.


	lno_t *partitionedPointCoordinates = new lno_t [numLocalCoords];
	lno_t *newpartitionedPointCoordinates = new lno_t [numLocalCoords];

	lno_t currentCoordCount = numLocalCoords;
	lno_t currentPartitionCount = 1;

	lno_t coordinateBegin = 0;
	lno_t coordinateEnd = numLocalCoords;

	bool firstIteration = true;
	size_t currentCutCoordinate = 0;

	lno_t *inTotalCounts = new lno_t [1];
	inTotalCounts[0] = numLocalCoords;
	lno_t *outTotalCounts = NULL;

	scalar_t *cutCoordinates =  allCutCoordinates;
	for (int i = 0; i < coordDim; ++i){
		lno_t partitionCoordinateBegin = 0;
		outTotalCounts = new lno_t[currentPartitionCount * partNo[i]];

		for (int j = 0; j < currentPartitionCount; ++j){

			pqJagged_getMinMaxCoord<scalar_t>(pqJagged_coordinates[i], minCoordinate,maxCoordinate,
					numLocalCoords, env, numThreads,
					firstIteration, partitionedPointCoordinates, coordinateBegin, coordinateEnd);

			pqJagged_1DPart_simple<scalar_t, lno_t>(pqJagged_coordinates[i], pqJagged_weights[0], pqJagged_uniformWeights[0],
					numLocalCoords, numGlobalCoords,	minCoordinate,
					maxCoordinate, pqJagged_uniformParts[0], pqJagged_partSizes[0], partNo[i], numThreads,
					imbalanceTolerance, cutCoordinates,
					firstIteration,partitionedPointCoordinates, coordinateBegin, coordinateEnd);

			getChunksFromCoordinates<lno_t,scalar_t>(partNo[i], numLocalCoords, numThreads,
					pqJagged_coordinates[i], cutCoordinates, outTotalCounts,
					partitionedPointCoordinates, firstIteration,newpartitionedPointCoordinates, coordinateBegin, coordinateEnd);
			partitionCoordinateBegin += coordinateEnd - coordinateBegin;
			//cutCoordinates += partNo[i] - 1;
			firstIteration = false;
#ifdef _debug
			for (size_t ii = 0;ii < partNo[i]; ++ii){
				cout << "cnt:" << totalCounts[ii] << " for i:" << ii << endl;
			}
			scalar_t _EPSILON = numeric_limits<scalar_t>::epsilon();
			for (size_t ii = 0;ii < partNo[i]; ++ii){

				lno_t begin = 0;
				if (ii > 0) begin = totalCounts[ii -1];
				scalar_t left = minCoordinate;
				scalar_t right = maxCoordinate;

				if (ii > 0) left = cutCoordinates[ii -1];
				if (ii < partNo[i] -1) right = cutCoordinates[ii];
				for (lno_t j = begin; j < totalCounts[ii]; ++j){

					if(pqJagged_coordinates[i][partitionedPointCoordinates[j]] < left || (pqJagged_coordinates[i][partitionedPointCoordinates[j]] >= right && !( fabs(pqJagged_coordinates[i][partitionedPointCoordinates[j]] - maxCoordinate) <= _EPSILON))){
						cout <<  "ERROR:" << partitionedPointCoordinates[j] << " is assigned to part:" << i << " with left:" << left << " and right:" << right <<  " its coordinate:" <<  pqJagged_coordinates[i][partitionedPointCoordinates[j]] << endl;

						exit(1);
					}
				}
			}
#endif

		}
		delete [] inTotalCounts;
		inTotalCounts = outTotalCounts;
		outTotalCounts = NULL;
		break;
	}












	////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////
	// The algorithm
	////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////

	partId_t part0 = 0;
	partId_t part1 = numGlobalParts-1;
	int sanityCheck = numGlobalParts;
	int groupSize = comm->getSize();
	int rank = comm->getRank();

	long imbalanceReductionFactor(1);
	long nparts = numGlobalParts;
	while ((nparts >>= 1) != 0) imbalanceReductionFactor++;

	imbalanceTolerance /= imbalanceReductionFactor;

	int iteration = 1;

	env->memory("RCB algorithm set up");
#ifdef RCBCODE
	env->timerStart(MACRO_TIMERS, "Parallel RCB");

	while (part1 > part0 && groupSize>1 && numGlobalCoords>0 && sanityCheck--){

		////////////////////////////////////////////////////////
		// Which coordinates are left and which are right?

		Array<unsigned char> lrflags(numLocalCoords);
		scalar_t cutValue=0;  // TODO eventually save this for user
		int cutDimension=0;
		scalar_t imbalance=0, weightLeft=0, weightRight=0;
		partId_t leftHalfNumParts=0;

		env->timerStart(MICRO_TIMERS, "Find cut", iteration, 2);

		try{
			determineCut<mvector_t, Adapter>(env, comm,
					params, numTestCuts, imbalanceTolerance,
					coordDim, mvector,
					uniformWeights.view(0,criteriaDim), mcnorm, solution,
					part0, part1,
					lrflags.view(0, numLocalCoords),
					cutDimension, cutValue, imbalance, leftHalfNumParts,
					weightLeft, weightRight);
		}
		Z2_FORWARD_EXCEPTIONS

		env->timerStop(MICRO_TIMERS, "Find cut", iteration, 2);

		// Do we have empty left or right halves?

		bool skipLeft = (weightLeft == 0);
		bool skipRight = (weightRight == 0);

		////////////////////////////////////////////////////////
		// Migrate the multivector of data.

		int leftHalfNumProcs=0;

		env->timerStart(MICRO_TIMERS, "Migrate", iteration, 2);

		try{ // on return mvector has my new data

			migrateData<mvector_t>( env, comm, lrflags.view(0,numLocalCoords),
					mvector, leftHalfNumProcs);
		}
		Z2_FORWARD_EXCEPTIONS

		env->timerStop(MICRO_TIMERS, "Migrate", iteration, 2);

		env->localBugAssertion(__FILE__, __LINE__, "num procs in half",
				leftHalfNumProcs > 0 && leftHalfNumProcs < groupSize,
				BASIC_ASSERTION);

		bool inLeftHalf = (rank < leftHalfNumProcs);

		if ((inLeftHalf && skipLeft) || (!inLeftHalf && skipRight)){
			groupSize = 1;
			numLocalCoords = 0;
			continue;
		}

		////////////////////////////////////////////////////////
		// Divide into two subgroups.

		env->timerStart(MICRO_TIMERS, "Create sub group, sub data", iteration, 2);

		int *ids = NULL;

		if (rank < leftHalfNumProcs){
			groupSize = leftHalfNumProcs;
			ids = new int [groupSize];
			env->localMemoryAssertion(__FILE__, __LINE__, groupSize, ids);
			for (int i=0; i < groupSize; i++)
				ids[i] = i;
			part1 = part0 + leftHalfNumParts - 1;
		}
		else {
			groupSize = comm->getSize() - leftHalfNumProcs;
			rank -= leftHalfNumProcs;
			ids = new int [groupSize];
			env->localMemoryAssertion(__FILE__, __LINE__, groupSize, ids);
			for (int i=0; i < groupSize; i++)
				ids[i] = i + leftHalfNumProcs;
			part0 += leftHalfNumParts;
		}

		ArrayView<const int> idView(ids, groupSize);
		// TODO - memory leak here.
		RCP<Comm<int> > subComm = comm->createSubcommunicator(idView);
		comm = subComm;

		delete [] ids;

		////////////////////////////////////////////////////////
		// Create a new multivector for my smaller group.

		ArrayView<const gno_t> gnoList = mvector->getMap()->getNodeElementList();
		size_t localSize = mvector->getLocalLength();

		// Tpetra will calculate the globalSize.
		size_t globalSize = Teuchos::OrdinalTraits<size_t>::invalid();

		RCP<map_t> subMap;
		try{
			subMap= rcp(new map_t(globalSize, gnoList, 0, comm));
		}
		Z2_THROW_OUTSIDE_ERROR(*env)


		typedef ArrayView<const scalar_t> coordList_t;
		coordList_t *avSubList = new coordList_t [multiVectorDim];

		for (int dim=0; dim < multiVectorDim; dim++)
			avSubList[dim] = mvector->getData(dim).view(0, localSize);

		ArrayRCP<const ArrayView<const scalar_t> > subVectors =
				arcp(avSubList, 0, multiVectorDim);

		RCP<mvector_t> subMvector;

		try{
			subMvector = rcp(new mvector_t(
					subMap, subVectors.view(0, multiVectorDim), multiVectorDim));
		}
		Z2_THROW_OUTSIDE_ERROR(*env)

		env->timerStop(MICRO_TIMERS, "Create sub group, sub data", iteration, 2);

		mvector = subMvector;

		numLocalCoords = mvector->getLocalLength();
		numGlobalCoords = mvector->getGlobalLength();

		iteration++;

		env->memory("New subgroup data created");
	}


	env->timerStop(MACRO_TIMERS, "Parallel RCB");

	env->localBugAssertion(__FILE__, __LINE__, "partitioning failure",
			sanityCheck, BASIC_ASSERTION);

	ArrayRCP<partId_t> partId;

	if (numLocalCoords > 0){
		partId_t *tmp = new partId_t [numLocalCoords];
		env->localMemoryAssertion(__FILE__, __LINE__, numLocalCoords, tmp);
		partId = arcp(tmp, 0, numLocalCoords, true);
	}

	env->memory("Solution array created");

	if ((part1 > part0) && (numLocalCoords > 0)){ // Serial partitioning

		// scalar_t cutValue;   TODO
		// int cutDimension;
		// scalar_t imbalance;

		env->timerStart(MACRO_TIMERS, "Serial RCB");

		try{
			ArrayView<lno_t> emptyIndex;

			serialRCB<mvector_t, Adapter>(env, 1, params,
					numTestCuts, imbalanceTolerance,
					coordDim, mvector, emptyIndex,
					uniformWeights.view(0,criteriaDim), solution,
					part0, part1, partId.view(0,numLocalCoords));
		}
		Z2_FORWARD_EXCEPTIONS

		env->timerStop(MACRO_TIMERS, "Serial RCB");

	}
	else{
		for (lno_t i=0; i < partId.size(); i++)
			partId[i] = part0;
	}

#endif
	////////////////////////////////////////////////////////
	// Done: Compute quality metrics and update the solution
	// TODO: The algorithm will not compute the metrics.
	// It provides the solution to the PartitioningSolution.
	// Metrics can be computed with a method that takes
	// a model and a solution.

#ifdef RCBCODE


	ArrayRCP<MetricValues<scalar_t> > metrics;
	partId_t numParts, numNonemptyParts;

	ArrayRCP<input_t> objWgt(new input_t [criteriaDim], 0, criteriaDim, true);
	Array<ArrayView<scalar_t> > partSizeArrays(criteriaDim);

	for (int wdim = 0, widx=coordDim; wdim < criteriaDim; wdim++){
		if (!uniformWeights[wdim]){
			objWgt[wdim] = input_t(mvector->getData(widx++), 1);
		}
		if (partSizes[wdim].size() > 0)
			partSizeArrays[wdim] =
					ArrayView<scalar_t>(partSizes[wdim].getRawPtr(), numGlobalParts);
	}

	// global call
	objectMetrics<scalar_t, lno_t>(
			env, problemComm, numGlobalParts,                  // input
			partSizeArrays.view(0, criteriaDim),               // input
			partId.view(0, numLocalCoords),                    // input
			objWgt.view(0, criteriaDim), mcnorm,               // input
			numParts, numNonemptyParts, metrics);              // output


	ArrayRCP<const gno_t> gnoList =
			arcpFromArrayView(mvector->getMap()->getNodeElementList());

	if (env->doStatus() && (numGlobalCoords < 500)){
		ostringstream oss;
		oss << "Solution: ";
		for (gno_t i=0; i < gnoList.size(); i++)
			oss << gnoList[i] << " (" << partId[i] << ") ";

		env->debug(VERBOSE_DETAILED_STATUS, oss.str());
	}


	solution->setParts(gnoList, partId, metrics);
#endif

	delete []cutCoordinates;
	delete []pqJagged_coordinates;
	delete []pqJagged_weights;
	delete []pqJagged_uniformParts;
	delete []pqJagged_partSizes;
	delete []pqJagged_uniformWeights;
}


} // namespace Zoltan2

#endif
