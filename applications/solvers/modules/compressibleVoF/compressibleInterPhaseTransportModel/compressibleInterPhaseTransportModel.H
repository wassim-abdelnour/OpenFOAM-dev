/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     | Website:  https://openfoam.org
    \\  /    A nd           | Copyright (C) 2017-2023 OpenFOAM Foundation
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

Class
    Foam::compressibleInterPhaseTransportModel

Description
    Transport model selection class for the compressibleInterFoam family of
    solvers.

    By default the standard mixture transport modelling approach is used in
    which a single momentum stress model (laminar, non-Newtonian, LES or RAS) is
    constructed for the mixture.  However if the \c simulationType in
    constant/momentumTransport is set to \c twoPhaseTransport the alternative
    Euler-Euler two-phase transport modelling approach is used in which separate
    stress models (laminar, non-Newtonian, LES or RAS) are instantiated for each
    of the two phases allowing for different modeling for the phases.

SourceFiles
    compressibleInterPhaseTransportModel.C

\*---------------------------------------------------------------------------*/

#ifndef compressibleInterPhaseTransportModel_H
#define compressibleInterPhaseTransportModel_H

#include "compressibleTwoPhaseVoFMixture.H"
#include "compressibleMomentumTransportModels.H"
#include "phaseCompressibleMomentumTransportModel.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

class compressibleInterPhaseThermophysicalTransportModel;

/*---------------------------------------------------------------------------*\
            Class compressibleInterPhaseTransportModel Declaration
\*---------------------------------------------------------------------------*/

class compressibleInterPhaseTransportModel
{
    // Private Data

        //- Switch to select two-phase or mixture transport modelling
        Switch twoPhaseTransport_;

        //- Two-phase mixture
        const compressibleTwoPhaseVoFMixture& mixture_;

        //- Mixture volumetric flux
        const surfaceScalarField& phi_;

        //- Phase volumetric flux
        const surfaceScalarField& alphaPhi1_;

        //- Phase-1 mass-flux (constructed for two-phase transport)
        const surfaceScalarField& alphaRhoPhi1_;

        //- Phase-2 mass-flux (constructed for two-phase transport)
        const surfaceScalarField& alphaRhoPhi2_;

        //- Mixture transport model (constructed for mixture transport)
        autoPtr<compressible::momentumTransportModel> mixtureMomentumTransport_;

        //- Phase-1 transport model (constructed for two-phase transport)
        autoPtr<phaseCompressible::momentumTransportModel> momentumTransport1_;

        //- Phase-2 transport model (constructed for two-phase transport)
        autoPtr<phaseCompressible::momentumTransportModel> momentumTransport2_;

        friend class compressibleInterPhaseThermophysicalTransportModel;


public:

    // Constructors

        //- Construct from components
        compressibleInterPhaseTransportModel
        (
            const volScalarField& rho,
            const volVectorField& U,
            const surfaceScalarField& phi,
            const surfaceScalarField& rhoPhi,
            const surfaceScalarField& alphaPhi1,
            const surfaceScalarField& alphaRhoPhi1,
            const surfaceScalarField& alphaRhoPhi2,
            const compressibleTwoPhaseVoFMixture& mixture
        );

        //- Disallow default bitwise copy construction
        compressibleInterPhaseTransportModel
        (
            const compressibleInterPhaseTransportModel&
        ) = delete;


    // Member Functions

        //- Return the effective momentum stress divergence
        tmp<fvVectorMatrix> divDevTau(volVectorField& U) const;

        //- Predict the phase or mixture transport models
        void predict();

        //- Correct the phase or mixture transport models
        void correct();


    // Member Operators

        //- Disallow default bitwise assignment
        void operator=(const compressibleInterPhaseTransportModel&) = delete;
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
