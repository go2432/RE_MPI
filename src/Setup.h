/*******************************************************************************
GPU OPTIMIZED MONTE CARLO (GOMC) 2.31
Copyright (C) 2018  GOMC Group
A copy of the GNU General Public License can be found in the COPYRIGHT.txt
along with this program, also can be found at <http://www.gnu.org/licenses/>.
********************************************************************************/
#ifndef SETUP_H
#define SETUP_H

#include <string> //for filename
#include <cstdlib>

#include "ConfigSetup.h"
#include "FFSetup.h"
#include "PDBSetup.h"
#include "PRNGSetup.h"
#include "MolSetup.h"
#include "ReplEx.h"

class Setup
{
public:
  //Read order follows each item
  ConfigSetup config;  //1
  PDBSetup pdb;        //2
  FFSetup ff;          //3
  PRNGSetup prng;      //4
  MolSetup mol;        //5

  void Init(char const*const configFileName, ReplicaExchangeParameters * replExParams)
  {
    //Read in all config data
    config.Init(configFileName, replExParams);
    //Read in FF data.
    ff.Init(config.in.files.param.name, config.in.ffKind.isCHARMM);
    //Read PDB dat
    pdb.Init(config.in.restart, config.in.files.pdb.name);
    //Read molecule data from psf
    prng.Init(config.in.restart, config.in.prng, config.in.files.seed.name);

    if(mol.Init(config.in.restart, config.in.files.psf.name) != 0) {
      exit(EXIT_FAILURE);
    }
    mol.AssignKinds(pdb.atoms, ff);

  }
};

#endif
