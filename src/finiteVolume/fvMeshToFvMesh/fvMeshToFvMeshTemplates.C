/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     | Website:  https://openfoam.org
    \\  /    A nd           | Copyright (C) 2022-2023 OpenFOAM Foundation
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

#include "fvMeshToFvMesh.H"
#include "directFvPatchFieldMapper.H"
#include "identityFvPatchFieldMapper.H"
#include "patchToPatchFvPatchFieldMapper.H"

// * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * * //

template<class Type>
void Foam::fvMeshToFvMesh::mapSrcToTgt
(
    const VolField<Type>& field,
    VolField<Type>& result
) const
{
    meshToMesh::mapSrcToTgt(field, result.primitiveFieldRef());

    typename VolField<Type>::
        Boundary& resultBf = result.boundaryFieldRef();

    forAll(srcToTgtPatchIDs(), i)
    {
        const label srcPatchi = srcToTgtPatchIDs()[i].first();
        const label tgtPatchi = srcToTgtPatchIDs()[i].second();

        const fvPatchField<Type>& srcField = field.boundaryField()[srcPatchi];
        fvPatchField<Type>& tgtField = resultBf[tgtPatchi];

        // Clone and map
        tmp<fvPatchField<Type>> tnewTgt
        (
            fvPatchField<Type>::New
            (
                srcField,
                tgtField.patch(),
                result(),
                patchToPatchFvPatchFieldMapper
                (
                    srcToTgtPatchToPatches()[i],
                    true
                )
            )
        );

        // Transfer all mapped quantities (value and e.g. gradient) onto
        // tgtField. Value will get overwritten below.
        tgtField.map(tnewTgt(), identityFvPatchFieldMapper());
    }

    forAll(tgtCuttingPatchIDs(), i)
    {
        const label patchi = tgtCuttingPatchIDs()[i];
        fvPatchField<Type>& pf = resultBf[patchi];
        pf == pf.patchInternalField();
    }
}


template<class Type>
Foam::tmp<Foam::VolField<Type>>
Foam::fvMeshToFvMesh::mapSrcToTgt
(
    const VolField<Type>& field
) const
{
    const fvMesh& tgtMesh = static_cast<const fvMesh&>(meshToMesh::tgtMesh());

    const fvBoundaryMesh& tgtBm = tgtMesh.boundary();
    const typename VolField<Type>::Boundary& srcBfld =
        field.boundaryField();

    PtrList<fvPatchField<Type>> tgtPatchFields(tgtBm.size());

    // construct tgt boundary patch types as copy of 'field' boundary types
    // note: this will provide place holders for fields with additional
    // entries, but these values will need to be reset
    forAll(srcToTgtPatchIDs(), i)
    {
        const label srcPatchi = srcToTgtPatchIDs()[i].first();
        const label tgtPatchi = srcToTgtPatchIDs()[i].second();

        if (!tgtPatchFields.set(tgtPatchi))
        {
            tgtPatchFields.set
            (
                tgtPatchi,
                fvPatchField<Type>::New
                (
                    srcBfld[srcPatchi],
                    tgtMesh.boundary()[tgtPatchi],
                    DimensionedField<Type, volMesh>::null(),
                    directFvPatchFieldMapper
                    (
                        labelList(tgtMesh.boundary()[tgtPatchi].size(), -1)
                    )
                )
            );
        }
    }

    // Any unset tgtPatchFields become calculated
    forAll(tgtPatchFields, tgtPatchi)
    {
        if (!tgtPatchFields.set(tgtPatchi))
        {
            // Note: use factory New method instead of direct generation of
            //       calculated so we keep constraints
            tgtPatchFields.set
            (
                tgtPatchi,
                fvPatchField<Type>::New
                (
                    calculatedFvPatchField<Type>::typeName,
                    tgtMesh.boundary()[tgtPatchi],
                    DimensionedField<Type, volMesh>::null()
                )
            );
        }
    }

    tmp<VolField<Type>> tresult
    (
        new VolField<Type>
        (
            IOobject
            (
                typedName("interpolate(" + field.name() + ")"),
                tgtMesh.time().name(),
                tgtMesh,
                IOobject::NO_READ,
                IOobject::NO_WRITE
            ),
            tgtMesh,
            field.dimensions(),
            Field<Type>(tgtMesh.nCells(), Zero),
            tgtPatchFields
        )
    );

    fvMeshToFvMesh::mapSrcToTgt<Type>(field, tresult.ref());

    return tresult;
}


template<class Type>
void Foam::fvMeshToFvMesh::mapSrcToTgt
(
    const VolInternalField<Type>& field,
    VolInternalField<Type>& result
) const
{
    meshToMesh::mapSrcToTgt(field, result);
}


template<class Type>
Foam::tmp<Foam::VolInternalField<Type>>
Foam::fvMeshToFvMesh::mapSrcToTgt
(
    const VolInternalField<Type>& field
) const
{
    const fvMesh& tgtMesh = static_cast<const fvMesh&>(meshToMesh::tgtMesh());

    tmp<VolInternalField<Type>> tresult
    (
        new VolInternalField<Type>
        (
            IOobject
            (
                typedName("interpolate(" + field.name() + ")"),
                tgtMesh.time().name(),
                tgtMesh,
                IOobject::NO_READ,
                IOobject::NO_WRITE
            ),
            tgtMesh,
            field.dimensions(),
            Field<Type>(tgtMesh.nCells(), Zero)
        )
    );

    fvMeshToFvMesh::mapSrcToTgt<Type>(field, tresult.ref());

    return tresult;
}


// ************************************************************************* //
