// This file is part of the Acts project.
//
// Copyright (C) 2019 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "Acts/Vertexing/KalmanVertexUpdater.hpp"
#include "Acts/Vertexing/VertexSmoother.hpp"
#include "Acts/Vertexing/VertexingError.hpp"

template <typename input_track_t, typename linearizer_t>
Acts::Result<void>
Acts::AdaptiveMultiVertexFitter<input_track_t, linearizer_t>::fit(
    State& state, const std::vector<Vertex<input_track_t>*>& verticesToFit,
    const linearizer_t& linearizer,
    const VertexFitterOptions<input_track_t>& vFitterOptions) const {
  // Set all vertices to fit in the current state
  state.vertexCollection = verticesToFit;

  // Perform fit on all vertices simultaneously
  auto fitRes = fitImpl(state, linearizer, vFitterOptions);

  if (!fitRes.ok()) {
    return fitRes.error();
  }

  return {};
}

template <typename input_track_t, typename linearizer_t>
Acts::Result<void>
Acts::AdaptiveMultiVertexFitter<input_track_t, linearizer_t>::fitImpl(
    State& state, const linearizer_t& linearizer,
    const VertexFitterOptions<input_track_t>& vFitterOptions) const {
  auto& geoContext = vFitterOptions.geoContext;

  // Reset annealing tool
  state.annealingState = AnnealingUtility::State();

  // Indicates how much the vertex positions have shifted
  // in last fit iteration. Will be false if vertex position
  // shift was too big. Needed if equilibrium is reached in
  // annealing procedure but fitter has not fully converged
  // yet and needs some more iterations until vertex position
  // shifts between iterations are small (converged).
  bool isSmallShift = true;

  // Number of iterations counter
  unsigned int nIter = 0;

  // Start iterating
  //while (nIter < m_cfg.maxIterations &&
   //      (!state.annealingState.equilibriumReached || !isSmallShift)) {

  while (nIter < 2 &&
         (!state.annealingState.equilibriumReached || !isSmallShift)) {
           
    // Initial loop over all vertices in state.vertexCollection
    int debugCount1 = 0;
    for (auto currentVtx : state.vertexCollection) {
      VertexInfo& currentVtxInfo = state.vtxInfoMap[currentVtx];
      currentVtxInfo.relinearize = false;
      //std::cout << "\tloop, iter=" << nIter << ",vtxloop=" <<debugCount1<<  std::endl;
      // Store old position of vertex, i.e. seed position
      // in case of first iteration or position determined
      // in previous iteration afterwards
      currentVtxInfo.oldPosition = currentVtx->fullPosition();

      //std::cout << "oldPosition set: " << currentVtxInfo.oldPosition << std::endl;

      // Determine if relinearization is needed
      if ((currentVtxInfo.oldPosition - currentVtxInfo.linPoint).norm() >
          m_cfg.maxDistToLinPoint) {

        //std::cout << "do relinearization" << std::endl;
        // Relinearization needed, distance too big
        currentVtxInfo.relinearize = true;
        // Prepare for fit with new vertex position
        prepareVertexForFit(state, currentVtx, vFitterOptions);
      }
      // Determine if constraint vertex exist
      if (state.vtxInfoMap[currentVtx]
              .constraintVertex.fullCovariance()
              != SpacePointSymMatrix::Zero()) {
        currentVtx->setPosition(
            state.vtxInfoMap[currentVtx].constraintVertex.fullPosition().template head<3>());
        currentVtx->setFitQuality(
            state.vtxInfoMap[currentVtx].constraintVertex.fitQuality());
        currentVtx->setCovariance(
            state.vtxInfoMap[currentVtx].constraintVertex.fullCovariance().template block<3,3>(0,0));


        //std::cout << "constraintPos set: " << state.vtxInfoMap[currentVtx].constraintVertex.fullPosition()<<std::endl;
        //std::cout << "constraintCov set: " << state.vtxInfoMap[currentVtx].constraintVertex.fullCovariance() << std::endl;
        //std::cout << "setFitQuality: " << state.vtxInfoMap[currentVtx].constraintVertex.fitQuality().first <<","<< state.vtxInfoMap[currentVtx].constraintVertex.fitQuality().second << std::endl;
      }

      else if (currentVtx->fullCovariance() == SpacePointSymMatrix::Zero()) {
        ////std::cout << "OUT!!!" << std::endl;
        return VertexingError::NoCovariance;
      }
      auto weight = 1. /
          m_cfg.annealingTool.getWeight(state.annealingState, 1.);
      //std::cout << "annealing weight: " << weight << std::endl;
      auto covAnn = currentVtx->fullCovariance() * weight;
      currentVtx->setCovariance(covAnn.template block<3,3>(0,0));

      //std::cout << "constraintPos ann:" << covAnn << std::endl;
      // Set vertexCompatibility for all TrackAtVertex objects
      // at current vertex
      setAllVertexCompatibilities(state, geoContext, currentVtx);
      debugCount1++;
      //std::cout << debugCount1 << ", trackloop vtxposition(): " << state.vertexCollection[0]->position() << std::endl;
    }  // End loop over vertex collection
    //std::cout << "end loop over vertex collection "<< std::endl;
    // Now after having estimated all compatibilities of all tracks at
    // all vertices, run again over all vertices to set track weights
    // and update the vertex
    //std::cout << "\n\nnIter: " << nIter << std::endl;
    setWeightsAndUpdate(state, linearizer);

    if (!state.annealingState.equilibriumReached) {
      ////std::cout << "anneal..." << std::endl;
      m_cfg.annealingTool.anneal(state.annealingState);
    }

    std::cout << nIter << ", NEW vtxposition(): " << state.vertexCollection[0]->position() << std::endl;

    isSmallShift = checkSmallShift(state);

    if(state.annealingState.equilibriumReached){
      //std::cout << nIter << ",equilibriumReached!" << std::endl;
    }
    else{
      //std::cout << nIter << ",not reached!" << std::endl;
    }
    if(isSmallShift){
      std::cout << nIter << ",isSmallShift!" << std::endl;
    }
    else{
      std::cout << nIter << "not smallshift" << std::endl;
    }

    ++nIter;
    std::cout << "\n\n\n" << std::endl;
  } 
  // Multivertex fit is finished

  // Check if smoothing is required
  if (m_cfg.doSmoothing) {
    for (auto vtx : state.vertexCollection) {
      // Smooth all tracks at vertex `vtx`
      auto smoothRes = VertexSmoothing::smoothVertexSequentially<input_track_t>(
          geoContext, vtx);
      if (!smoothRes.ok()) {
        return smoothRes.error();
      }
    }
  }

  return {};
}

template <typename input_track_t, typename linearizer_t>
Acts::Result<void>
Acts::AdaptiveMultiVertexFitter<input_track_t, linearizer_t>::addVtxToFit(
    State& state, Vertex<input_track_t>& newVertex,
    const linearizer_t& linearizer,
    const VertexFitterOptions<input_track_t>& vFitterOptions) const {
  if (newVertex.tracks().empty()) {
    return VertexingError::EmptyInput;
  }

  std::vector<Vertex<input_track_t>*> verticesToFit;

  // Prepares vtx and tracks for fast estimation method of their
  // compatibility with vertex
  auto res = prepareVertexForFit(state, &newVertex, vFitterOptions);
  if (!res.ok()) {
    return res.error();
  }

  // List of vertices added in last iteration
  std::vector<Vertex<input_track_t>*> lastIterAddedVertices = {&newVertex};
  // List of vertices added in current iteration
  std::vector<Vertex<input_track_t>*> currentIterAddedVertices;

  // Loop as long as new vertices are found that share tracks with
  // previously added vertices
  while (!lastIterAddedVertices.empty()) {
    for (auto& lastVtxIter : lastIterAddedVertices) {
      // Loop over all track at current lastVtxIter
      for (const TrackAtVertex<input_track_t>& trackIter :
           lastVtxIter->tracks()) {
        // Retrieve list of links to all vertices that currently use the current
        // track
        std::vector<Vertex<input_track_t>*>& linksToVertices =
            state.trkInfoMap[trackIter.id].linksToVertices;

        // Loop over all attached vertices and add those to vertex fit
        // which are not already in `verticesToFit`
        for (auto newVtxIter : linksToVertices) {
          if (!isAlreadyInList(newVtxIter, verticesToFit)) {
            // Add newVtxIter to verticesToFit
            verticesToFit.push_back(newVtxIter);

            // Add newVtxIter vertex to currentIterAddedVertices
            // if vertex != lastVtxIter
            if (newVtxIter != lastVtxIter) {
              currentIterAddedVertices.push_back(newVtxIter);
            }
          }
        }  // End for loop over linksToVertices
      }
    }  // End loop over lastIterAddedVertices

    lastIterAddedVertices = currentIterAddedVertices;
    currentIterAddedVertices.clear();
  }  // End while loop

  state.vertexCollection = verticesToFit;

  // Perform fit on all added vertices
  auto fitRes = fitImpl(state, linearizer, vFitterOptions);

  if (!fitRes.ok()) {
    return fitRes.error();
  }

  return {};
}

template <typename input_track_t, typename linearizer_t>
bool Acts::AdaptiveMultiVertexFitter<input_track_t, linearizer_t>::
    isAlreadyInList(
        Vertex<input_track_t>* vtx,
        const std::vector<Vertex<input_track_t>*>& verticesVec) const {
  return std::find(verticesVec.begin(), verticesVec.end(), vtx) !=
         verticesVec.end();
}

template <typename input_track_t, typename linearizer_t>
Acts::Result<void> Acts::
    AdaptiveMultiVertexFitter<input_track_t, linearizer_t>::prepareVertexForFit(
        State& state, Vertex<input_track_t>* vtx,
        const VertexFitterOptions<input_track_t>& vFitterOptions) const {
  const Vector3D& seedPos = state.vtxInfoMap[vtx].seedPosition.template head<3>();
  auto& geoContext = vFitterOptions.geoContext;

  // Loop over all tracks at current vertex
  for (const auto& trkAtVtx : vtx->tracks()) {
    auto res = m_cfg.ipEst.getParamsAtClosestApproach(
        geoContext, m_extractParameters(trkAtVtx.originalTrack), seedPos);
    if (!res.ok()) {
      return res.error();
    }
    // Set ip3dParams for current trackAtVertex
    state.trkInfoMap[trkAtVtx.id].ip3dParams = std::move(res.value());
  }
  return {};
}

template <typename input_track_t, typename linearizer_t>
Acts::Result<void>
Acts::AdaptiveMultiVertexFitter<input_track_t, linearizer_t>::
    setAllVertexCompatibilities(State& state, const GeometryContext& geoContext,
                                Vertex<input_track_t>* currentVtx) const {
  VertexInfo& currentVtxInfo = state.vtxInfoMap[currentVtx];
  // Create empty list of new TrackAtVertex objects
  // to be filled below. Needed due to constness of
  // tracksAtVertex list at vertex
  std::vector<TrackAtVertex<input_track_t>> newTracks;
  newTracks.reserve(currentVtx->tracks().size());

  // Loop over tracks at current vertex and
  // estimate compatibility with vertex
  for (auto& trkAtVtx : currentVtx->tracks()) {
    // Recover from cases where linearization point != 0 but
    // more tracks were added later on
    if (!state.trkInfoMap[trkAtVtx.id].ip3dParams) {
      auto res = m_cfg.ipEst.getParamsAtClosestApproach(
          geoContext, m_extractParameters(trkAtVtx.originalTrack),
          VectorHelpers::position(currentVtxInfo.linPoint));
      if (!res.ok()) {
        return res.error();
      }
      // Set ip3dParams for current trackAtVertex
      auto value = std::move(res.value());
      ////std::cout << "v " << value->position()[0] << " " << value->position()[1] << " " << value->position()[2] << std::endl;
      ////std::cout << "l 5 6" << std::endl;

      ////std::cout << "\nglobal position after extrapolation: " << value->position() << std::endl;
    
      ////std::cout << "ipEstParams: " <<  value->parameters()<<std::endl;
      state.trkInfoMap[trkAtVtx.id].ip3dParams = std::move(value);
    }

    // Create copy of current trackAtVertex in order
    // to modify it below
    newTracks.push_back(trkAtVtx);
    TrackAtVertex<input_track_t>* newTrkPtr = &(newTracks.back());

    ////std::cout << "inputGetComp, params: " << state.trkInfoMap[trkAtVtx.id].ip3dParams->parameters()<<std::endl;
    ////std::cout << "inputGetComp, position: " <<VectorHelpers::position(currentVtxInfo.oldPosition)<<std::endl;
    // Set compatibility with current vertex
    auto compRes = m_cfg.ipEst.getVertexCompatibility(
        geoContext, state.trkInfoMap[trkAtVtx.id].ip3dParams.get(),
        VectorHelpers::position(currentVtxInfo.oldPosition));

    if (!compRes.ok()) {
      return compRes.error();
    }

    double comp = *compRes;
    //std::cout << "vtxComp: " << comp << std::endl;
    newTrkPtr->vertexCompatibility = *compRes;
  }
  //std::cout << "setAllVertexCompatibilities done." << std::endl;
  // Set list of updated tracks to current vertex
  currentVtx->setTracksAtVertex(newTracks);
  return {};
}

template <typename input_track_t, typename linearizer_t>
Acts::Result<void> Acts::AdaptiveMultiVertexFitter<
    input_track_t, linearizer_t>::setWeightsAndUpdate(State& state,
                                                      const linearizer_t&
                                                          linearizer) const {
  for (auto vtx : state.vertexCollection) {  
    // Create empty list of new TrackAtVertex objects
    // to be filled below. Needed due to constness of
    // tracksAtVertex list at vertex
    std::vector<TrackAtVertex<input_track_t>> newTracks;
    newTracks.reserve(vtx->tracks().size());

    auto oldTracks = vtx->tracks();

    std::cout << "number of track at vtx: " << vtx->tracks().size() << std::endl;
    std::cout << "first trk: weight: " << oldTracks[0].trackWeight << std::endl;

    for (const auto& trkAtVtx : vtx->tracks()) {
      std::cout << "### TRACK ID: " << trkAtVtx.id << std::endl;
      std::cout << "\t### TRACK weight: " << trkAtVtx.trackWeight << std::endl;
    } 

    for (const auto& trkAtVtx : oldTracks) {
      std::cout << "### LOOP TRACK ID: " << trkAtVtx.id << std::endl;
      // Create copy of current trackAtVertex in order
      // to modify it below
      newTracks.push_back(trkAtVtx);
      TrackAtVertex<input_track_t>* newTrkPtr = &(newTracks.back());
      // Get all compatibilities of track to all vertices it is attached to
      auto collectRes = collectTrackToVertexCompatibilities(state, trkAtVtx);
      if (!collectRes.ok()) {
        return collectRes.error();
      }   

      for(auto& r : *collectRes){
        //std::cout << "compVec: " << r << std::endl;
      }
      //std::cout << "currentComp: " << trkAtVtx.vertexCompatibility << std::endl;
      std::cout << "weight before: " << newTrkPtr->trackWeight << std::endl;
      std::cout << "trkAtVtx.vertexCompatibility: " << trkAtVtx.vertexCompatibility << std::endl;
      // Set trackWeight for current track
      newTrkPtr->trackWeight = m_cfg.annealingTool.getWeight(
          state.annealingState, trkAtVtx.vertexCompatibility, *collectRes);
      std::cout << "weight after: " << newTrkPtr->trackWeight << std::endl;

      std::cout << "weight/minWeight: " << newTrkPtr->trackWeight  << ", " << m_cfg.minWeight << std::endl;
      if (newTrkPtr->trackWeight > m_cfg.minWeight) {
        // Check if linearization state exists or need to be relinearized
        if (newTrkPtr->linearizedState.covarianceAtPCA ==
                BoundSymMatrix::Zero() ||
            state.vtxInfoMap[vtx].relinearize) {
          const auto& origParams =
              m_extractParameters(newTrkPtr->originalTrack);
          auto result = linearizer.linearizeTrack(
              &origParams, state.vtxInfoMap[vtx].oldPosition);
          if (!result.ok()) {
            return result.error();
          }
          newTrkPtr->linearizedState = *result;
          state.vtxInfoMap[vtx].linPoint = state.vtxInfoMap[vtx].oldPosition;
        }
        // std::cout << "before kalman vtxposition(): " << state.vertexCollection[0]->position() << std::endl;
        // std::cout << "trkAtVtx: weight " << newTrkPtr->trackWeight << std::endl;
        // std::cout << "trkAtVtx: params " << newTrkPtr->fittedParams.parameters() << std::endl;
        // std::cout << "trkAtVtx: linparams " << newTrkPtr->linearizedState.parametersAtPCA << std::endl;
        // std::cout << "trkAtVtx: linPCA " << newTrkPtr->linearizedState.positionAtPCA << std::endl;
        // std::cout << "trkAtVtx: linPCAMom " << newTrkPtr->linearizedState.momentumAtPCA << std::endl;
        // std::cout << "trkAtVtx: linPoint " << newTrkPtr->linearizedState.linearizationPoint << std::endl;

        std::cout << "running updater now..." << std::endl;
        // Update the vertex with the new track
        auto updateRes =
            KalmanVertexUpdater::updateVertexWithTrack<input_track_t>(
                vtx, newTrkPtr);
        if (!updateRes.ok()) {
          return updateRes.error();
        }
      } else {
        ACTS_VERBOSE("Track weight too low. Skip track.");
      }
      //std::cout << "trackloop vtxposition(): " << state.vertexCollection[0]->position() << std::endl;
    }  // End loop over tracks at vertex

    vtx->setTracksAtVertex(newTracks);

    for (const auto& trkAtVtx : vtx->tracks()) {
      std::cout << "### TRACK ID: " << trkAtVtx.id << std::endl;
      std::cout << "\t### TRACK weight: " << trkAtVtx.trackWeight << std::endl;
    } 
    //std::cout << "NEW vtx->fullPosition(): " << vtx->fullPosition() << std::endl;

    ACTS_VERBOSE("New vertex position: " << vtx->fullPosition());
  }  // End loop over vertex collection

  //std::cout << "setWeightsAndUpdate done." << std::endl;
  return {};
}

template <typename input_track_t, typename linearizer_t>
Acts::Result<std::vector<double>>
Acts::AdaptiveMultiVertexFitter<input_track_t, linearizer_t>::
    collectTrackToVertexCompatibilities(
        State& state, const TrackAtVertex<input_track_t>& trk) const {
  // All vertices that currently hold the track `trk`
  std::vector<Vertex<input_track_t>*> vertices =
      state.trkInfoMap[trk.id].linksToVertices;

  // Vector to store all compatibility values, it will have
  // exactly the size of `vertices`(one value for each vertex
  // the track is attached to)
  std::vector<double> trkToVtxCompatibilities;
  trkToVtxCompatibilities.reserve(vertices.size());

  for (Vertex<input_track_t>* vtxPtr : vertices) {
    // find current track in list of tracks at vertex
    const auto& trkIter = std::find_if(
        vtxPtr->tracks().begin(), vtxPtr->tracks().end(),
        [&trk, this](auto& trkAtVtx) {
          return this->m_extractParameters(trkAtVtx.originalTrack) ==
                 this->m_extractParameters(trk.originalTrack);
        });
    if (trkIter == vtxPtr->tracks().end()) {
      return VertexingError::ElementNotFound;
    }
    // store vertexCompatibility of track to current vertex
    trkToVtxCompatibilities.push_back(trkIter->vertexCompatibility);
  }
  return trkToVtxCompatibilities;
}

template <typename input_track_t, typename linearizer_t>
bool Acts::AdaptiveMultiVertexFitter<
    input_track_t, linearizer_t>::checkSmallShift(State& state) const {
      //std::cout << "entering checkSmallShift" << std::endl;
  for (auto vtx : state.vertexCollection) {
    //std::cout << "state.vtxInfoMap[vtx].oldPosition:" << state.vtxInfoMap[vtx].oldPosition << std::endl;
    //std::cout << "vtx->fullPosition():" << vtx->fullPosition() << std::endl;
    auto diff =
        state.vtxInfoMap[vtx].oldPosition.template head<3>() - vtx->fullPosition().template head<3>();
    const auto& vtxWgt = vtx->fullCovariance().template block<3,3>(0,0).inverse();
    std::cout << "shift pos: " << diff << std::endl;
    std::cout << "shift cov: " << vtxWgt << std::endl;
    double relativeShift = diff.dot(vtxWgt * diff);
    std::cout << "shift/max: " << relativeShift << ", " << m_cfg.maxRelativeShift << std::endl;
    //std::cout << "checkSmallShift done." << std::endl;
    if (relativeShift > m_cfg.maxRelativeShift) {
      return false;
    }
  }
  return true;
}
