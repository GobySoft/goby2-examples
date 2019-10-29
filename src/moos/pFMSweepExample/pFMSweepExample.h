// Copyright 2009-2018 Toby Schneider (http://gobysoft.org/index.wt/people/toby)
//                     GobySoft, LLC (2013-)
//                     Massachusetts Institute of Technology (2007-2014)
//
//
// This file is part of the Goby Underwater Autonomy Project Binaries
// ("The Goby Binaries").
//
// The Goby Binaries are free software: you can redistribute them and/or modify
// them under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// The Goby Binaries are distributed in the hope that they will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Goby.  If not, see <http://www.gnu.org/licenses/>.

#ifndef pFMSweepExample20191029H
#define pFMSweepExample20191029H

#include <goby/moos/goby_moos_app.h>

#include "config.pb.h"

class FMSweepExample : public GobyMOOSApp
{
  public:
    static FMSweepExample* get_instance();
    static void delete_instance();

  private:
    FMSweepExample(FMSweepExampleConfig& cfg);
    ~FMSweepExample();

    void loop(); // from GobyMOOSApp

    void handle_db_time(const CMOOSMsg& msg);

  private:
    FMSweepExampleConfig& cfg_;
    static FMSweepExample* inst_;
};

#endif
