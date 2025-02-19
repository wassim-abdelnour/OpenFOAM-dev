/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     | Website:  https://openfoam.org
    \\  /    A nd           | Copyright (C) 2012-2022 OpenFOAM Foundation
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

\*---------------------------------------------------------------------------*/

#include "constantRadiation.H"
#include "addToRunTimeSelectionTable.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
namespace surfaceFilmModels
{

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

defineTypeNameAndDebug(constantRadiation, 0);

addToRunTimeSelectionTable
(
    radiationModel,
    constantRadiation,
    dictionary
);

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

constantRadiation::constantRadiation
(
    surfaceFilm& film,
    const dictionary& dict
)
:
    radiationModel(typeName, film, dict),
    qrConst_
    (
        IOobject
        (
            typedName("qrConst"),
            film.time().name(),
            film.mesh(),
            IOobject::MUST_READ,
            IOobject::AUTO_WRITE
        ),
        film.mesh()
    ),
    mask_
    (
        IOobject
        (
            typedName("mask"),
            film.time().name(),
            film.mesh(),
            IOobject::READ_IF_PRESENT,
            IOobject::AUTO_WRITE
        ),
        film.mesh(),
        dimensionedScalar(dimless, 1.0)
    ),
    absorptivity_(coeffDict_.lookup<scalar>("absorptivity")),
    timeStart_(coeffDict_.lookup<scalar>("timeStart")),
    duration_(coeffDict_.lookup<scalar>("duration"))
{
    mask_ = pos0(mask_);
}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

constantRadiation::~constantRadiation()
{}


// * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * * //

void constantRadiation::correct()
{}


tmp<volScalarField::Internal> constantRadiation::Shs()
{
    const scalar time = film().time().userTimeValue();

    if ((time >= timeStart_) && (time <= timeStart_ + duration_))
    {
        return volScalarField::Internal::New
        (
            typedName("Shs"),
            mask_*qrConst_*filmModel_.coverage()*absorptivity_
        );
    }
    else
    {
        return volScalarField::Internal::New
        (
            typedName("Shs"),
            film().mesh(),
            dimensionedScalar(dimMass/pow3(dimTime), 0)
        );
    }
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace surfaceFilmModels
} // End namespace Foam

// ************************************************************************* //
