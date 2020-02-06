// This file is part of the Acts project.
//
// Copyright (C) 2016-2020 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "Acts/Surfaces/StrawSurface.hpp"
#include "Acts/Surfaces/PolyhedronRepresentation.hpp"

#include <iomanip>
#include <iostream>
#include <utility>

#include "Acts/Surfaces/InfiniteBounds.hpp"

Acts::StrawSurface::StrawSurface(std::shared_ptr<const Transform3D> htrans,
                                 double radius, double halez)
    : GeometryObject(), LineSurface(std::move(htrans), radius, halez) {}

Acts::StrawSurface::StrawSurface(std::shared_ptr<const Transform3D> htrans,
                                 std::shared_ptr<const LineBounds> lbounds)
    : GeometryObject(), LineSurface(std::move(htrans), std::move(lbounds)) {}

Acts::StrawSurface::StrawSurface(
    const std::shared_ptr<const LineBounds>& lbounds,
    const DetectorElementBase& detelement)
    : GeometryObject(), LineSurface(lbounds, detelement) {}

Acts::StrawSurface::StrawSurface(const Acts::StrawSurface& other)
    : GeometryObject(), LineSurface(other) {}

Acts::StrawSurface::StrawSurface(const GeometryContext& gctx,
                                 const StrawSurface& other,
                                 const Transform3D& transf)
    : GeometryObject(), LineSurface(gctx, other, transf) {}

Acts::StrawSurface& Acts::StrawSurface::operator=(const StrawSurface& other) {
  if (this != &other) {
    LineSurface::operator=(other);
    m_bounds = other.m_bounds;
  }
  return *this;
}

std::shared_ptr<Acts::StrawSurface> Acts::StrawSurface::clone(
    const GeometryContext& gctx, const Transform3D& shift) const {
  return std::shared_ptr<StrawSurface>(this->clone_impl(gctx, shift));
}

Acts::StrawSurface* Acts::StrawSurface::clone_impl(
    const GeometryContext& gctx, const Transform3D& shift) const {
  return new StrawSurface(gctx, *this, shift);
}

Acts::PolyhedronRepresentation Acts::StrawSurface::polyhedronRepresentation(
    const GeometryContext& gctx, size_t l0div, size_t /*l1div*/) const {
  std::vector<Vector3D> vertices;
  std::vector<std::vector<size_t>> faces;

  if (l0div == 1) {
    throw std::domain_error("Polyhedron repr of straw with 1 div is undefined");
  }

  double phistep = 2 * M_PI / l0div;
  double hlZ = m_bounds->halflengthZ();
  double r = m_bounds->r();

  Vector3D left(r, 0, -hlZ);
  Vector3D right(r, 0, hlZ);

  const Transform3D& sfTransform = transform(gctx);

  for (size_t i = 0; i < l0div; i++) {
    Transform3D rot(AngleAxis3D(i * phistep, Vector3D::UnitZ()));
    vertices.push_back(sfTransform * rot * left);
    vertices.push_back(sfTransform * rot * right);
  }

  for (size_t v = 0; v < vertices.size() - 2; v = v + 2) {
    faces.push_back({v, v + 1, v + 3, v + 2});
  }
  if (l0div > 2) {
    faces.push_back({vertices.size() - 2, vertices.size() - 1, 1, 0});
  }

  return PolyhedronRepresentation(vertices, faces);
}
