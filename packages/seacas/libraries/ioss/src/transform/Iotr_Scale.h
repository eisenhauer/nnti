// Copyright(C) 1999-2010
// Sandia Corporation. Under the terms of Contract
// DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government retains
// certain rights in this software.
//         
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// 
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
// 
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
//     * Neither the name of Sandia Corporation nor the names of its
//       contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef IOSS_Iotr_Scale_h
#define IOSS_Iotr_Scale_h

#include <Ioss_Transform.h>             // for Transform, Factory
#include <string>                       // for string
#include "Ioss_VariableType.h"          // for VariableType
namespace Ioss { class Field; }

namespace Ioss {
}

namespace Iotr {

  class Scale_Factory : public Factory
    {
    public:
      static const Scale_Factory* factory();
    private:
      Scale_Factory();
      Ioss::Transform* make(const std::string&) const;
    };

  class Scale: public Ioss::Transform
    {
      friend class Scale_Factory;

    public:
      const Ioss::VariableType *output_storage(const Ioss::VariableType *in) const;
      int output_count(int in) const;

      void set_property(const std::string &name, int value);
      void set_property(const std::string &name, double value);

    protected:
      Scale();

      bool internal_execute(const Ioss::Field &field, void *data);

    private:
      int    intMultiplier;
      double realMultiplier;
    };
}

#endif // IOSS_Iotr_Scale_h
