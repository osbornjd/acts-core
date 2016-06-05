// This file is part of the ACTS project.
//
// Copyright (C) 2016 ACTS project team
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

///////////////////////////////////////////////////////////////////
// SurfaceBounds.cpp, ACTS project
///////////////////////////////////////////////////////////////////

// Trk
#include "ACTS/Surfaces/SurfaceBounds.hpp"

std::ostream&
Acts::operator<<(std::ostream& sl, const SurfaceBounds& sb)
{
  return sb.dump(sl);
}
