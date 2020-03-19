// This file is part of the Acts project.
//
// Copyright (C) 2019 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

template <typename vfitter_t>
auto Acts::ZScanVertexFinder<vfitter_t>::find(
    const std::vector<const InputTrack_t*>& trackVector,
    const VertexFinderOptions<InputTrack_t>& vFinderOptions) const
    -> Result<std::vector<Vertex<InputTrack_t>>> {
  // Determine if we use constraint or not
  bool useConstraint = false;
  if (vFinderOptions.vertexConstraint.fullCovariance().determinant() != 0) {
    useConstraint = true;
  }

  double ZResult = 0.;
  // Prepare the vector of points, on which the 3d mode has later to be
  // calculated
  std::vector<std::pair<double, double>> zPositions;

  for (const auto& iTrk : trackVector) {
    // Extract BoundParameters from InputTrack_t object
    const BoundParameters& params = m_extractParameters(*iTrk);

    std::pair<double, double> z0AndWeight;
    ImpactParametersAndSigma ipas;
    if (useConstraint &&
        vFinderOptions.vertexConstraint.covariance()(0, 0) != 0) {
      auto estRes =
          m_cfg.ipEstimator.estimate(params, vFinderOptions.vertexConstraint);
      if (estRes.ok()) {
        ipas = *estRes;
      } else {
        return estRes.error();
      }
    }

    if (ipas.sigmad0 > 0) {
      // calculate z0
      z0AndWeight.first =
          ipas.IPz0 + vFinderOptions.vertexConstraint.position().z();

      // calculate chi2 of IP
      double chi2IP = std::pow(ipas.IPd0 / ipas.sigmad0, 2);

      if (!m_cfg.disableAllWeights) {
        z0AndWeight.second =
            1. / (1. + std::exp((chi2IP - m_cfg.constraintcutoff) /
                                m_cfg.constrainttemp));
        // overflow protection
        if (!std::isnormal(z0AndWeight.second)) {
          z0AndWeight.second = 0.;
        }
      } else {
        z0AndWeight.second = 1.;
      }
    } else {
      ACTS_DEBUG(
          "Unable to compute IP significance. "
          "Setting IP weight to 1.");

      z0AndWeight.first = params.position()[eZ];
      z0AndWeight.second = 1.;
    }

    // apply pT weighting as/if configured
    if (!m_cfg.disableAllWeights && (m_cfg.usePt || m_cfg.useLogPt)) {
      double Pt = std::abs(1. / params.parameters()[ParID_t::eQOP]) *
                  std::sin(params.parameters()[ParID_t::eTHETA]);
      if (m_cfg.usePt) {
        z0AndWeight.second *= std::pow(Pt, m_cfg.expPt);
      } else {
        z0AndWeight.second *=
            Pt > m_cfg.minPt ? std::log(Pt / m_cfg.minPt) : 0.;
      }
    }

    if (z0AndWeight.second >= m_cfg.minWeight) {
      zPositions.push_back(z0AndWeight);
    }
  }  // end of loop over perigeeList

  if (!zPositions.empty()) {
    auto res = m_cfg.mode1dFinder.getMode(zPositions);
    if (res.ok()) {
      ZResult = *res;
    } else {
      return res.error();
    }

    ACTS_DEBUG("Resulting mean Z position found: " << ZResult);
  }

  // constraint x()/y() equals 0 if no constraint
  SpacePointVector output(vFinderOptions.vertexConstraint.position().x(),
                          vFinderOptions.vertexConstraint.position().y(),
                          ZResult, vFinderOptions.vertexConstraint.time());
  Vertex<InputTrack_t> vtxResult = Vertex<InputTrack_t>(output);

  // Vector to be filled with one single vertex
  std::vector<Vertex<InputTrack_t>> vertexCollection;

  // Add vertex to vertexCollection
  vertexCollection.push_back(vtxResult);

  return vertexCollection;
}
