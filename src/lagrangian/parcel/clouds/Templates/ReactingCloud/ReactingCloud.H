/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     | Website:  https://openfoam.org
    \\  /    A nd           | Copyright (C) 2011-2022 OpenFOAM Foundation
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
    Foam::ReactingCloud

Description
    Templated base class for reacting cloud

    - Adds to thermodynamic cloud
      - Variable composition (single phase)
      - Phase change

SourceFiles
    ReactingCloudI.H
    ReactingCloud.C

\*---------------------------------------------------------------------------*/

#ifndef ReactingCloud_H
#define ReactingCloud_H

#include "volFieldsFwd.H"
#include "fvMatricesFwd.H"
#include "dimensionedTypes.H"
#include "fvMesh.H"
#include "fluidThermo.H"
#include "Cloud.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// Forward declaration of classes

template<class CloudType>
class PhaseChangeModel;


/*---------------------------------------------------------------------------*\
                      Class ReactingCloudName Declaration
\*---------------------------------------------------------------------------*/

TemplateName(ReactingCloud);


/*---------------------------------------------------------------------------*\
                        Class ReactingCloud Declaration
\*---------------------------------------------------------------------------*/

template<class CloudType>
class ReactingCloud
:
    public CloudType,
    public ReactingCloudName
{
public:

    // Public Typedefs

        //- Type of cloud this cloud was instantiated for
        typedef CloudType cloudType;

        //- Type of parcel the cloud was instantiated for
        typedef typename CloudType::particleType parcelType;

        //- Convenience typedef for this cloud type
        typedef ReactingCloud<CloudType> reactingCloudType;


private:

    // Private Data

        //- Cloud copy pointer
        autoPtr<ReactingCloud<CloudType>> cloudCopyPtr_;



protected:

    // Protected data

        //- Parcel constant properties
        typename parcelType::constantProperties constProps_;


        // References to the cloud sub-models

            //- Reacting phase change model
            autoPtr<PhaseChangeModel<ReactingCloud<CloudType>>>
                phaseChangeModel_;


        // Sources

            //- Mass transfer fields - one per carrier phase specie
            PtrList<volScalarField::Internal> rhoTrans_;


    // Protected Member Functions

        // New parcel helper functions

            //- Check that size of a composition field is valid
            void checkSuppliedComposition
            (
                const scalarField& YSupplied,
                const scalarField& Y,
                const word& YName
            );


        // Initialisation

            //- Set cloud sub-models
            void setModels();


        // Cloud evolution functions

            //- Reset state of cloud
            void cloudReset(ReactingCloud<CloudType>& c);


public:

    // Constructors

        //- Construct given carrier fields and thermo
        ReactingCloud
        (
            const word& cloudName,
            const volScalarField& rho,
            const volVectorField& U,
            const dimensionedVector& g,
            const fluidThermo& carrierThermo,
            const bool readFields = true
        );

        //- Copy constructor with new name
        ReactingCloud(ReactingCloud<CloudType>& c, const word& name);

        //- Copy constructor with new name - creates bare cloud
        ReactingCloud
        (
            const fvMesh& mesh,
            const word& name,
            const ReactingCloud<CloudType>& c
        );

        //- Disallow default bitwise copy construction
        ReactingCloud(const ReactingCloud&) = delete;

        //- Construct and return clone based on (this) with new name
        virtual autoPtr<Cloud<parcelType>> clone(const word& name)
        {
            return autoPtr<Cloud<parcelType>>
            (
                new ReactingCloud(*this, name)
            );
        }

        //- Construct and return bare clone based on (this) with new name
        virtual autoPtr<Cloud<parcelType>> cloneBare(const word& name) const
        {
            return autoPtr<Cloud<parcelType>>
            (
                new ReactingCloud(this->mesh(), name, *this)
            );
        }


    //- Destructor
    virtual ~ReactingCloud();


    // Member Functions

        // Access

            //- Return a reference to the cloud copy
            inline const ReactingCloud& cloudCopy() const;

            //- Return the constant properties
            inline const typename parcelType::constantProperties&
                constProps() const;

            //- Return access to the constant properties
            inline typename parcelType::constantProperties& constProps();


            // Sub-models

                //- Return const access to reacting phase change model
                inline const PhaseChangeModel<ReactingCloud<CloudType>>&
                    phaseChange() const;

                //- Return reference to reacting phase change model
                inline PhaseChangeModel<ReactingCloud<CloudType>>&
                    phaseChange();


            // Sources

                //- Mass

                    //- Return reference to mass source for field i
                    inline volScalarField::Internal&
                        rhoTrans(const label i);

                    //- Return const access to mass source fields
                    inline const PtrList<volScalarField::Internal>&
                        rhoTrans() const;

                    //- Return reference to mass source fields
                    inline PtrList<volScalarField::Internal>&
                        rhoTrans();

                    //- Return mass source term for specie i - specie eqn
                    inline tmp<fvScalarMatrix> SYi
                    (
                        const label i,
                        const volScalarField& Yi
                    ) const;

                    //- Return tmp total mass source for carrier phase
                    //  - fully explicit
                    inline tmp<volScalarField::Internal> Srho() const;

                    //- Return total mass source term [kg/m^3/s]
                    inline tmp<fvScalarMatrix> Srho
                    (
                        const volScalarField& rho
                    ) const;


        // Cloud evolution functions

            //- Set parcel thermo properties
            void setParcelThermoProperties(parcelType& parcel);

            //- Check parcel properties
            void checkParcelProperties
            (
                parcelType& parcel,
                const bool fullyDescribed
            );

            //- Store the current cloud state
            void storeState();

            //- Reset the current cloud to the previously stored state
            void restoreState();

            //- Reset the cloud source terms
            void resetSourceTerms();

            //- Apply relaxation to (steady state) cloud sources
            void relaxSources(const ReactingCloud<CloudType>& cloudOldTime);

            //- Apply scaling to (transient) cloud sources
            void scaleSources();

            //- Evolve the cloud
            void evolve();


        // I-O

            //- Print cloud information
            void info();


    // Member Operators

        //- Disallow default bitwise assignment
        void operator=(const ReactingCloud&) = delete;
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#include "ReactingCloudI.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#ifdef NoRepository
    #include "ReactingCloud.C"
#endif

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
