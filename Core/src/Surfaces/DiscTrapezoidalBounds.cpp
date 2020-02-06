// This file is part of the Acts project.
//
// Copyright (C) 2016-2020 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "Acts/Surfaces/DiscTrapezoidalBounds.hpp"

#include <cmath>
#include <iomanip>
#include <iostream>

#include "Acts/Utilities/detail/periodic.hpp"

Acts::DiscTrapezoidalBounds::DiscTrapezoidalBounds(double minhalfx,
                                                   double maxhalfx, double maxR,
                                                   double minR, double avephi,
                                                   double stereo)
    : m_rMin(std::min(std::abs(minR), std::abs(maxR))),
      m_rMax(std::max(std::abs(minR), std::abs(maxR))),
      m_minHalfX(std::abs(minhalfx)),
      m_maxHalfX(std::abs(maxhalfx)),
      m_avgPhi(detail::radian_sym(avephi)),
      m_stereo(stereo) {}

Acts::DiscTrapezoidalBounds::~DiscTrapezoidalBounds() = default;

Acts::DiscTrapezoidalBounds* Acts::DiscTrapezoidalBounds::clone() const {
  return new DiscTrapezoidalBounds(*this);
}

Acts::SurfaceBounds::BoundsType Acts::DiscTrapezoidalBounds::type() const {
  return SurfaceBounds::DiscTrapezoidal;
}

std::vector<TDD_real_t> Acts::DiscTrapezoidalBounds::valueStore() const {
  std::vector<TDD_real_t> values(DiscTrapezoidalBounds::bv_length);
  values[bv_rMin] = rMin();
  values[bv_rMax] = rMax();
  values[bv_minHalfX] = minHalflengthX();
  values[bv_maxHalfX] = maxHalflengthX();
  values[bv_averagePhi] = averagePhi();
  values[bv_stereo] = m_stereo;
  return values;
}

Acts::Vector2D Acts::DiscTrapezoidalBounds::toLocalCartesian(
    const Acts::Vector2D& lposition) const {
  return {lposition[eLOC_R] * std::sin(lposition[eLOC_PHI] - m_avgPhi),
          lposition[eLOC_R] * std::cos(lposition[eLOC_PHI] - m_avgPhi)};
}

Acts::ActsMatrixD<2, 2> Acts::DiscTrapezoidalBounds::jacobianToLocalCartesian(
    const Acts::Vector2D& lposition) const {
  ActsMatrixD<2, 2> jacobian;
  jacobian(0, eLOC_R) = std::sin(lposition[eLOC_PHI] - m_avgPhi);
  jacobian(1, eLOC_R) = std::cos(lposition[eLOC_PHI] - m_avgPhi);
  jacobian(0, eLOC_PHI) = lposition[eLOC_R] * std::cos(lposition[eLOC_PHI]);
  jacobian(1, eLOC_PHI) = lposition[eLOC_R] * -std::sin(lposition[eLOC_PHI]);
  return jacobian;
}

bool Acts::DiscTrapezoidalBounds::inside(
    const Acts::Vector2D& lposition, const Acts::BoundaryCheck& bcheck) const {
  Vector2D vertices[] = {{minHalflengthX(), rMin()},
                         {maxHalflengthX(), rMax()},
                         {-maxHalflengthX(), rMax()},
                         {-minHalflengthX(), rMin()}};
  auto jacobian = jacobianToLocalCartesian(lposition);
  return bcheck.transformed(jacobian).isInside(toLocalCartesian(lposition),
                                               vertices);
}

double Acts::DiscTrapezoidalBounds::distanceToBoundary(
    const Acts::Vector2D& lposition) const {
  Vector2D vertices[] = {{minHalflengthX(), rMin()},
                         {maxHalflengthX(), rMax()},
                         {-maxHalflengthX(), rMax()},
                         {-minHalflengthX(), rMin()}};
  return BoundaryCheck(true).distance(toLocalCartesian(lposition), vertices);
}

std::vector<Acts::Vector2D> Acts::DiscTrapezoidalBounds::vertices(
    unsigned int /*lseg*/) const {
  return {};
}

// ostream operator overload
std::ostream& Acts::DiscTrapezoidalBounds::toStream(std::ostream& sl) const {
  sl << std::setiosflags(std::ios::fixed);
  sl << std::setprecision(7);
  sl << "Acts::DiscTrapezoidalBounds:  (innerRadius, outerRadius, hMinX, "
        "hMaxX, hlengthY, hPhiSector, averagePhi, rCenter, stereo) = ";
  sl << "(" << rMin() << ", " << rMax() << ", " << minHalflengthX() << ", "
     << maxHalflengthX() << ", " << halflengthY() << ", " << halfPhiSector()
     << ", " << averagePhi() << ", " << rCenter() << ", " << stereo() << ")";
  sl << std::setprecision(-1);
  return sl;
}
