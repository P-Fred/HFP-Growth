#pragma once
/*
 *   Copyright (C) 2017,  CentraleSupelec
 *
 *   Author : Frédéric Pennerath 
 *
 *   Contributor :
 *
 *   This library is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public
 *   License (GPL) as published by the Free Software Foundation; either
 *   version 3 of the License, or any later version.
 *   
 *   This library is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 *   General Public License for more details.
 *   
 *   You should have received a copy of the GNU General Public
 *   License along with this library; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 *   Contact : frederic.pennerath@centralesupelec.fr
 *
 */

namespace cool {
  template<typename Container> struct ContainerManager {
    static void reserve(Container& container, size_t n) {}
  };

  template<typename V, typename A> struct ContainerManager<std::vector<V,A>> {
    static void reserve(std::vector<V,A>& container, size_t n) {
      container.reserve(n);
    }
  };

  template<typename Container>
  void reserve(Container& container, size_t n) {
    ContainerManager<Container>::reserve(container, n);
  }  
}
