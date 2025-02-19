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

\*---------------------------------------------------------------------------*/

#include "SLTSDdtScheme.H"
#include "surfaceInterpolate.H"
#include "fvMatrices.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace fv
{

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

template<class Type>
void SLTSDdtScheme<Type>::relaxedDiag
(
    scalarField& rD,
    const surfaceScalarField& phi
) const
{
    const labelUList& owner = mesh().owner();
    const labelUList& neighbour = mesh().neighbour();
    scalarField diag(rD.size(), 0.0);

    forAll(owner, facei)
    {
        if (phi[facei] > 0.0)
        {
            diag[owner[facei]] += phi[facei];
            rD[neighbour[facei]] += phi[facei];
        }
        else
        {
            diag[neighbour[facei]] -= phi[facei];
            rD[owner[facei]] -= phi[facei];
        }
    }

    forAll(phi.boundaryField(), patchi)
    {
        const fvsPatchScalarField& pphi = phi.boundaryField()[patchi];
        const labelUList& faceCells = pphi.patch().patch().faceCells();

        forAll(pphi, patchFacei)
        {
            if (pphi[patchFacei] > 0.0)
            {
                diag[faceCells[patchFacei]] += pphi[patchFacei];
            }
            else
            {
                rD[faceCells[patchFacei]] -= pphi[patchFacei];
            }
        }
    }

    rD += (1.0/alpha_ - 2.0)*diag;
}


template<class Type>
tmp<volScalarField> SLTSDdtScheme<Type>::SLrDeltaT() const
{
    const surfaceScalarField& phi =
        mesh().objectRegistry::template
            lookupObject<surfaceScalarField>(phiName_);

    const dimensionedScalar& deltaT = mesh().time().deltaT();

    tmp<volScalarField> trDeltaT
    (
        volScalarField::New
        (
            "rDeltaT",
            mesh(),
            dimensionedScalar(dimless/dimTime, 0),
            extrapolatedCalculatedFvPatchScalarField::typeName
        )
    );

    volScalarField& rDeltaT = trDeltaT.ref();

    relaxedDiag(rDeltaT, phi);

    if (phi.dimensions() == dimensionSet(0, 3, -1, 0, 0))
    {
        rDeltaT.primitiveFieldRef() = max
        (
            rDeltaT.primitiveField()/mesh().V(),
            scalar(1)/deltaT.value()
        );
    }
    else if (phi.dimensions() == dimensionSet(1, 0, -1, 0, 0))
    {
        const volScalarField& rho =
            mesh().objectRegistry::template lookupObject<volScalarField>
            (
                rhoName_
            ).oldTime();

        rDeltaT.primitiveFieldRef() = max
        (
            rDeltaT.primitiveField()/(rho.primitiveField()*mesh().V()),
            scalar(1)/deltaT.value()
        );
    }
    else
    {
        FatalErrorInFunction
            << "Incorrect dimensions of phi: " << phi.dimensions()
            << abort(FatalError);
    }

    rDeltaT.correctBoundaryConditions();

    return trDeltaT;
}


template<class Type>
tmp<VolField<Type>>
SLTSDdtScheme<Type>::fvcDdt
(
    const dimensioned<Type>& dt
)
{
    const volScalarField rDeltaT(SLrDeltaT());

    const word ddtName("ddt("+dt.name()+')');

    if (mesh().moving())
    {
        tmp<VolField<Type>> tdtdt
        (
            VolField<Type>::New
            (
                ddtName,
                mesh(),
                dimensioned<Type>
                (
                    "0",
                    dt.dimensions()/dimTime,
                    Zero
                )
            )
        );

        tdtdt.ref().primitiveFieldRef() =
            rDeltaT.primitiveField()*dt.value()*(1.0 - mesh().V0()/mesh().V());

        return tdtdt;
    }
    else
    {
        return tmp<VolField<Type>>
        (
            VolField<Type>::New
            (
                ddtName,
                mesh(),
                dimensioned<Type>
                (
                    "0",
                    dt.dimensions()/dimTime,
                    Zero
                ),
                calculatedFvPatchField<Type>::typeName
            )
        );
    }
}


template<class Type>
tmp<VolField<Type>>
SLTSDdtScheme<Type>::fvcDdt
(
    const VolField<Type>& vf
)
{
    const volScalarField rDeltaT(SLrDeltaT());

    const word ddtName("ddt("+vf.name()+')');

    if (mesh().moving())
    {
        return tmp<VolField<Type>>
        (
            VolField<Type>::New
            (
                ddtName,
                rDeltaT()*(vf() - vf.oldTime()()*mesh().V0()/mesh().V()),
                rDeltaT.boundaryField()*
                (
                    vf.boundaryField() - vf.oldTime().boundaryField()
                )
            )
        );
    }
    else
    {
        return tmp<VolField<Type>>
        (
            VolField<Type>::New
            (
                ddtName,
                rDeltaT*(vf - vf.oldTime())
            )
        );
    }
}


template<class Type>
tmp<VolField<Type>>
SLTSDdtScheme<Type>::fvcDdt
(
    const dimensionedScalar& rho,
    const VolField<Type>& vf
)
{
    const volScalarField rDeltaT(SLrDeltaT());

    const word ddtName("ddt("+rho.name()+','+vf.name()+')');

    if (mesh().moving())
    {
        return tmp<VolField<Type>>
        (
            VolField<Type>::New
            (
                ddtName,
                rDeltaT()*rho*(vf() - vf.oldTime()()*mesh().V0()/mesh().V()),
                rDeltaT.boundaryField()*rho.value()*
                (
                    vf.boundaryField() - vf.oldTime().boundaryField()
                )
            )
        );
    }
    else
    {
        return tmp<VolField<Type>>
        (
            VolField<Type>::New
            (
                ddtName,
                rDeltaT*rho*(vf - vf.oldTime())
            )
        );
    }
}


template<class Type>
tmp<VolField<Type>>
SLTSDdtScheme<Type>::fvcDdt
(
    const volScalarField& rho,
    const VolField<Type>& vf
)
{
    const volScalarField rDeltaT(SLrDeltaT());

    const word ddtName("ddt("+rho.name()+','+vf.name()+')');

    if (mesh().moving())
    {
        return tmp<VolField<Type>>
        (
            VolField<Type>::New
            (
                ddtName,
                rDeltaT()*
                (
                    rho()*vf()
                  - rho.oldTime()()*vf.oldTime()()*mesh().V0()/mesh().V()
                ),
                rDeltaT.boundaryField()*
                (
                    rho.boundaryField()*vf.boundaryField()
                  - rho.oldTime().boundaryField()
                   *vf.oldTime().boundaryField()
                )
            )
        );
    }
    else
    {
        return tmp<VolField<Type>>
        (
            VolField<Type>::New
            (
                ddtName,
                rDeltaT*(rho*vf - rho.oldTime()*vf.oldTime())
            )
        );
    }
}


template<class Type>
tmp<VolField<Type>>
SLTSDdtScheme<Type>::fvcDdt
(
    const volScalarField& alpha,
    const volScalarField& rho,
    const VolField<Type>& vf
)
{
    const volScalarField rDeltaT(SLrDeltaT());

    const word ddtName("ddt("+alpha.name()+','+rho.name()+','+vf.name()+')');

    if (mesh().moving())
    {
        return tmp<VolField<Type>>
        (
            VolField<Type>::New
            (
                ddtName,
                rDeltaT()*
                (
                    alpha()*rho()*vf()
                  - alpha.oldTime()()*rho.oldTime()()*vf.oldTime()()
                   *mesh().Vsc0()/mesh().Vsc()
                ),
                rDeltaT.boundaryField()*
                (
                    alpha.boundaryField()
                   *rho.boundaryField()
                   *vf.boundaryField()

                  - alpha.oldTime().boundaryField()
                   *rho.oldTime().boundaryField()
                   *vf.oldTime().boundaryField()
                )
            )
        );
    }
    else
    {
        return tmp<VolField<Type>>
        (
            VolField<Type>::New
            (
                ddtName,
                rDeltaT
               *(
                   alpha*rho*vf - alpha.oldTime()*rho.oldTime()*vf.oldTime()
                )
            )
        );
    }
}


template<class Type>
tmp<fvMatrix<Type>>
SLTSDdtScheme<Type>::fvmDdt
(
    const VolField<Type>& vf
)
{
    tmp<fvMatrix<Type>> tfvm
    (
        new fvMatrix<Type>
        (
            vf,
            vf.dimensions()*dimVol/dimTime
        )
    );

    fvMatrix<Type>& fvm = tfvm.ref();

    const scalarField rDeltaT(SLrDeltaT()().primitiveField());

    fvm.diag() = rDeltaT*mesh().V();

    if (mesh().moving())
    {
        fvm.source() = rDeltaT*vf.oldTime().primitiveField()*mesh().V0();
    }
    else
    {
        fvm.source() = rDeltaT*vf.oldTime().primitiveField()*mesh().V();
    }

    return tfvm;
}


template<class Type>
tmp<fvMatrix<Type>>
SLTSDdtScheme<Type>::fvmDdt
(
    const dimensionedScalar& rho,
    const VolField<Type>& vf
)
{
    tmp<fvMatrix<Type>> tfvm
    (
        new fvMatrix<Type>
        (
            vf,
            rho.dimensions()*vf.dimensions()*dimVol/dimTime
        )
    );
    fvMatrix<Type>& fvm = tfvm.ref();

    const scalarField rDeltaT(SLrDeltaT()().primitiveField());

    fvm.diag() = rDeltaT*rho.value()*mesh().V();

    if (mesh().moving())
    {
        fvm.source() = rDeltaT
            *rho.value()*vf.oldTime().primitiveField()*mesh().V0();
    }
    else
    {
        fvm.source() = rDeltaT
            *rho.value()*vf.oldTime().primitiveField()*mesh().V();
    }

    return tfvm;
}


template<class Type>
tmp<fvMatrix<Type>>
SLTSDdtScheme<Type>::fvmDdt
(
    const volScalarField& rho,
    const VolField<Type>& vf
)
{
    tmp<fvMatrix<Type>> tfvm
    (
        new fvMatrix<Type>
        (
            vf,
            rho.dimensions()*vf.dimensions()*dimVol/dimTime
        )
    );
    fvMatrix<Type>& fvm = tfvm.ref();

    const scalarField rDeltaT(SLrDeltaT()().primitiveField());

    fvm.diag() = rDeltaT*rho.primitiveField()*mesh().V();

    if (mesh().moving())
    {
        fvm.source() = rDeltaT
            *rho.oldTime().primitiveField()
            *vf.oldTime().primitiveField()*mesh().V0();
    }
    else
    {
        fvm.source() = rDeltaT
            *rho.oldTime().primitiveField()
            *vf.oldTime().primitiveField()*mesh().V();
    }

    return tfvm;
}


template<class Type>
tmp<fvMatrix<Type>>
SLTSDdtScheme<Type>::fvmDdt
(
    const volScalarField& alpha,
    const volScalarField& rho,
    const VolField<Type>& vf
)
{
    tmp<fvMatrix<Type>> tfvm
    (
        new fvMatrix<Type>
        (
            vf,
            alpha.dimensions()*rho.dimensions()*vf.dimensions()*dimVol/dimTime
        )
    );
    fvMatrix<Type>& fvm = tfvm.ref();

    const scalarField rDeltaT(SLrDeltaT()().primitiveField());

    fvm.diag() =
        rDeltaT*alpha.primitiveField()*rho.primitiveField()*mesh().Vsc();

    if (mesh().moving())
    {
        fvm.source() = rDeltaT
            *alpha.oldTime().primitiveField()
            *rho.oldTime().primitiveField()
            *vf.oldTime().primitiveField()*mesh().Vsc0();
    }
    else
    {
        fvm.source() = rDeltaT
            *alpha.oldTime().primitiveField()
            *rho.oldTime().primitiveField()
            *vf.oldTime().primitiveField()*mesh().Vsc();
    }

    return tfvm;
}


template<class Type>
tmp<typename SLTSDdtScheme<Type>::fluxFieldType>
SLTSDdtScheme<Type>::fvcDdtUfCorr
(
    const VolField<Type>& U,
    const SurfaceField<Type>& Uf
)
{
    const surfaceScalarField rDeltaT(fvc::interpolate(SLrDeltaT()));

    fluxFieldType phiUf0(mesh().Sf() & Uf.oldTime());
    fluxFieldType phiCorr
    (
        phiUf0 - fvc::dotInterpolate(mesh().Sf(), U.oldTime())
    );

    return fluxFieldType::New
    (
        "ddtCorr(" + U.name() + ',' + Uf.name() + ')',
        this->fvcDdtPhiCoeff(U.oldTime(), phiUf0, phiCorr)
       *rDeltaT*phiCorr
    );
}


template<class Type>
tmp<typename SLTSDdtScheme<Type>::fluxFieldType>
SLTSDdtScheme<Type>::fvcDdtPhiCorr
(
    const VolField<Type>& U,
    const fluxFieldType& phi
)
{
    const surfaceScalarField rDeltaT(fvc::interpolate(SLrDeltaT()));

    fluxFieldType phiCorr
    (
        phi.oldTime() - fvc::dotInterpolate(mesh().Sf(), U.oldTime())
    );

    return fluxFieldType::New
    (
        "ddtCorr(" + U.name() + ',' + phi.name() + ')',
        this->fvcDdtPhiCoeff(U.oldTime(), phi.oldTime(), phiCorr)
       *rDeltaT*phiCorr
    );
}


template<class Type>
tmp<typename SLTSDdtScheme<Type>::fluxFieldType>
SLTSDdtScheme<Type>::fvcDdtUfCorr
(
    const volScalarField& rho,
    const VolField<Type>& U,
    const SurfaceField<Type>& Uf
)
{
    const surfaceScalarField rDeltaT(fvc::interpolate(SLrDeltaT()));

    if
    (
        U.dimensions() == dimVelocity
     && Uf.dimensions() == dimDensity*dimVelocity
    )
    {
        VolField<Type> rhoU0
        (
            rho.oldTime()*U.oldTime()
        );

        fluxFieldType phiUf0(mesh().Sf() & Uf.oldTime());
        fluxFieldType phiCorr(phiUf0 - fvc::dotInterpolate(mesh().Sf(), rhoU0));

        return fluxFieldType::New
        (
            "ddtCorr(" + rho.name() + ',' + U.name() + ',' + Uf.name() + ')',
            this->fvcDdtPhiCoeff(rhoU0, phiUf0, phiCorr, rho.oldTime())
           *rDeltaT*phiCorr
        );
    }
    else if
    (
        U.dimensions() == dimDensity*dimVelocity
     && Uf.dimensions() == dimDensity*dimVelocity
    )
    {
        fluxFieldType phiUf0(mesh().Sf() & Uf.oldTime());
        fluxFieldType phiCorr
        (
            phiUf0 - fvc::dotInterpolate(mesh().Sf(), U.oldTime())
        );

        return fluxFieldType::New
        (
            "ddtCorr(" + rho.name() + ',' + U.name() + ',' + Uf.name() + ')',
            this->fvcDdtPhiCoeff
            (
                U.oldTime(),
                phiUf0,
                phiCorr,
                rho.oldTime()
            )*rDeltaT*phiCorr
        );
    }
    else
    {
        FatalErrorInFunction
            << "dimensions of Uf are not correct"
            << abort(FatalError);

        return fluxFieldType::null();
    }
}


template<class Type>
tmp<typename SLTSDdtScheme<Type>::fluxFieldType>
SLTSDdtScheme<Type>::fvcDdtPhiCorr
(
    const volScalarField& rho,
    const VolField<Type>& U,
    const fluxFieldType& phi
)
{
    const dimensionedScalar rDeltaT = 1.0/mesh().time().deltaT();

    if
    (
        U.dimensions() == dimVelocity
     && phi.dimensions() == rho.dimensions()*dimFlux
    )
    {
        VolField<Type> rhoU0
        (
            rho.oldTime()*U.oldTime()
        );

        fluxFieldType phiCorr
        (
            phi.oldTime() - fvc::dotInterpolate(mesh().Sf(), rhoU0)
        );

        return fluxFieldType::New
        (
            "ddtCorr(" + rho.name() + ',' + U.name() + ',' + phi.name() + ')',
            this->fvcDdtPhiCoeff
            (
                rhoU0,
                phi.oldTime(),
                phiCorr,
                rho.oldTime()
            )*rDeltaT*phiCorr
        );
    }
    else if
    (
        U.dimensions() == rho.dimensions()*dimVelocity
     && phi.dimensions() == rho.dimensions()*dimFlux
    )
    {
        fluxFieldType phiCorr
        (
            phi.oldTime() - fvc::dotInterpolate(mesh().Sf(), U.oldTime())
        );

        return fluxFieldType::New
        (
            "ddtCorr(" + rho.name() + ',' + U.name() + ',' + phi.name() + ')',
            this->fvcDdtPhiCoeff
            (
                U.oldTime(),
                phi.oldTime(),
                phiCorr,
                rho.oldTime()
            )*rDeltaT*phiCorr
        );
    }
    else
    {
        FatalErrorInFunction
            << "dimensions of phi are not correct"
            << abort(FatalError);

        return fluxFieldType::null();
    }
}


template<class Type>
tmp<surfaceScalarField> SLTSDdtScheme<Type>::meshPhi
(
    const VolField<Type>&
)
{
    return surfaceScalarField::New
    (
        "meshPhi",
        mesh(),
        dimensionedScalar(dimVolume/dimTime, 0)
    );
}


template<class Type>
tmp<scalarField> SLTSDdtScheme<Type>::meshPhi
(
    const VolField<Type>&,
    const label patchi
)
{
    return tmp<scalarField>
    (
        new scalarField(mesh().boundary()[patchi].size(), 0)
    );
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace fv

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// ************************************************************************* //
