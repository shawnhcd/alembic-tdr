//-*****************************************************************************
//
// Copyright (c) 2013,
//  Sony Pictures Imageworks, Inc. and
//  Industrial Light & Magic, a division of Lucasfilm Entertainment Company Ltd.
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// *       Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// *       Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
// *       Neither the name of Sony Pictures Imageworks, nor
// Industrial Light & Magic nor the names of their contributors may be used
// to endorse or promote products derived from this software without specific
// prior written permission.
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
//
//-*****************************************************************************

#include <Alembic/AbcCoreOgawa/ReadUtil.h>
#include <cstddef>
#include <cstdlib>
#include <zstd.h>

#if defined(_MSC_VER)
#  if defined(max)
#    undef max
#  endif
#  if defined(min)
#    undef min
#  endif
#endif

#include <half.h>

namespace Alembic {
namespace AbcCoreOgawa {
namespace ALEMBIC_VERSION_NS {

//-*****************************************************************************
void
ReadDimensions( Ogawa::IDataPtr iDims,
                Ogawa::IDataPtr iData,
                size_t iThreadId,
                const AbcA::DataType &iDataType,
                Util::Dimensions & oDim )
{
    if ( iData->getSize() < 16 )
    {
        oDim = Util::Dimensions( 0 );
    }
    // find it based on of the size of the data
    else if ( iDims->getSize() == 0 )
    {
        std::size_t numItems =
            ( iData->getSize() - 16 ) / iDataType.getNumBytes();

        // for misshaped data bump up our dimensions by 1 so we have
        // more allocated for the partial read
        if (( iData->getSize() - 16 ) % iDataType.getNumBytes() != 0)
        {
            numItems += 1;
        }

        oDim = Util::Dimensions( numItems );
    }
    // we need to read our dimensions
    else
    {

        // we write them as uint64_t so / 8
        std::size_t numRanks = iDims->getSize() / 8;

        oDim.setRank( numRanks );

        std::vector< Util::uint64_t > dims( numRanks );
        if ( dims.empty() )
        {
            return;
        }

        iDims->read( numRanks * 8, &( dims.front() ), 0, iThreadId );
        for ( std::size_t i = 0; i < numRanks; ++i )
        {
            oDim[i] = dims[i];
        }

        // we have less data than what the dimensions suggest
        // we should, so calculate them based on what we have
        if ( iDataType.getPod() != Alembic::Util::kStringPOD &&
             iDataType.getPod() != Alembic::Util::kWstringPOD &&
             (iDataType.getNumBytes() * oDim.numPoints() >
                iData->getSize() - 16) )
        {
            std::size_t numItems =
                ( iData->getSize() - 16 ) / iDataType.getNumBytes();

            // for misshaped data bump up our dimensions by 1 so we have
            // more allocated for the partial read
            if (( iData->getSize() - 16 ) % iDataType.getNumBytes() != 0)
            {
                numItems += 1;
            }

            oDim = Util::Dimensions( numItems );
        }
    }
}

//-*****************************************************************************
void
ReadTDRDimensions( Ogawa::IDataPtr iDims,
                Ogawa::IDataPtr iData,
                size_t iThreadId,
                const AbcA::DataType &iDataType,
                Util::Dimensions & oDim )
{
    if ( iData->getSize() < 8 )
    {
        oDim = Util::Dimensions( 0 );
    }
    // find it based on of the size of the data
    else if ( iDims->getSize() == 0 )
    {
        // read the origin data size
        std::size_t originDataSize = 0;
        iData->read(8, &originDataSize, 0, 0);
        std::size_t numItems =
            originDataSize / iDataType.getNumBytes();

        // for misshaped data bump up our dimensions by 1 so we have
        // more allocated for the partial read
        if (originDataSize % iDataType.getNumBytes() != 0)
        {
            numItems += 1;
        }

        oDim = Util::Dimensions( numItems );
    }
    // we need to read our dimensions
    else
    {
        // read the origin data size
        std::size_t originDataSize = 0;
        iData->read(8, &originDataSize, 0, 0);

        // we write them as uint64_t so / 8
        std::size_t numRanks = iDims->getSize() / 8;

        oDim.setRank( numRanks );

        std::vector< Util::uint64_t > dims( numRanks );
        if ( dims.empty() )
        {
            return;
        }

        iDims->read( numRanks * 8, &( dims.front() ), 0, iThreadId );
        for ( std::size_t i = 0; i < numRanks; ++i )
        {
            oDim[i] = dims[i];
        }

        // we have less data than what the dimensions suggest
        // we should, so calculate them based on what we have
        if ( iDataType.getPod() != Alembic::Util::kStringPOD &&
             iDataType.getPod() != Alembic::Util::kWstringPOD &&
             (iDataType.getNumBytes() * oDim.numPoints() >
                originDataSize ) )
        {
            std::size_t numItems =
                originDataSize / iDataType.getNumBytes();

            // for misshaped data bump up our dimensions by 1 so we have
            // more allocated for the partial read
            if (originDataSize % iDataType.getNumBytes() != 0)
            {
                numItems += 1;
            }

            oDim = Util::Dimensions( numItems );
        }
    }
}

//-*****************************************************************************
template < typename FROMPOD >
void ConvertToBool( char * fromBuffer, void * toBuffer, std::size_t iSize )
{
    std::size_t numConvert = iSize / sizeof( FROMPOD );

    FROMPOD * fromPodBuffer = ( FROMPOD * ) ( fromBuffer );
    Util::bool_t * toPodBuffer = (Util::bool_t *) ( toBuffer );

    for ( std::size_t i = 0; i < numConvert; ++i )
    {
        Util::bool_t t = ( fromPodBuffer[i] != 0 );
        toPodBuffer[i] = t;
    }

}

//-*****************************************************************************
template < typename TOPOD >
void ConvertFromBool( char * fromBuffer, void * toBuffer, std::size_t iSize )
{
    // bool_t is stored as 1 bytes so iSize really is the size of the array

    TOPOD * toPodBuffer = ( TOPOD * ) ( toBuffer );

    // do it backwards so we don't accidentally clobber over ourself
    for ( std::size_t i = iSize; i > 0; --i )
    {
        TOPOD t = static_cast< TOPOD >( fromBuffer[i-1] != 0 );
        toPodBuffer[i-1] = t;
    }

}

//-*****************************************************************************
template < typename TOPOD >
void getMinAndMax(TOPOD & iMin, TOPOD & iMax)
{
    iMin = std::numeric_limits<TOPOD>::min();
    iMax = std::numeric_limits<TOPOD>::max();
}

//-*****************************************************************************
template <>
void getMinAndMax<Util::float16_t>(
    Util::float16_t & iMin, Util::float16_t & iMax )
{
    iMax = HALF_MAX;
    iMin = -iMax;
}

//-*****************************************************************************
template <>
void getMinAndMax<Util::float32_t>(
    Util::float32_t & iMin, Util::float32_t & iMax )
{
    iMax = std::numeric_limits<Util::float32_t>::max();
    iMin = -iMax;
}

//-*****************************************************************************
template <>
void getMinAndMax<Util::float64_t>(
    Util::float64_t & iMin, Util::float64_t & iMax )
{
    iMax = std::numeric_limits<Util::float64_t>::max();
    iMin = -iMax;
}

//-*****************************************************************************
template < typename FROMPOD, typename TOPOD >
void ConvertData( char * fromBuffer, void * toBuffer, std::size_t iSize )
{
    std::size_t numConvert = iSize / sizeof( FROMPOD );

    FROMPOD * fromPodBuffer = ( FROMPOD * ) ( fromBuffer );
    TOPOD * toPodBuffer = ( TOPOD * ) ( toBuffer );

    if ( sizeof( FROMPOD ) > sizeof( TOPOD ) )
    {
        // get the min and max of the smaller TOPOD type
        TOPOD toPodMin = 0;
        TOPOD toPodMax = 0;
        getMinAndMax< TOPOD >( toPodMin, toPodMax );

        // cast it back into the larger FROMPOD
        FROMPOD podMin = static_cast< FROMPOD >( toPodMin );
        FROMPOD podMax = static_cast< FROMPOD >( toPodMax );

        // handle from signed to unsigned wrap case
        if ( podMin > podMax )
        {
            podMin = 0;
        }

        for ( std::size_t i = 0; i < numConvert; ++i )
        {
            FROMPOD f = fromPodBuffer[i];
            if ( f < podMin )
            {
                f = podMin;
            }
            else if ( f > podMax )
            {
                f = podMax;
            }
            TOPOD t = static_cast< TOPOD >( f );
            toPodBuffer[i] = t;
        }
    }
    else
    {
        TOPOD toPodMin = 0;
        TOPOD toPodMax = 0;
        getMinAndMax< TOPOD >( toPodMin, toPodMax);

        FROMPOD podMin = 0;
        FROMPOD podMax = 0;
        getMinAndMax< FROMPOD >( podMin, podMax);

        if ( podMin != 0 && toPodMin == 0 )
        {
            podMin = 0;
        }
        // adjust max when converting to signed from unsigned of the same
        // sized integral
        else if ( podMin == 0 && toPodMin != 0 &&
                  sizeof( FROMPOD ) == sizeof( TOPOD ) )
        {
            podMax = static_cast< FROMPOD >( toPodMax );
        }

        // do it backwards so we don't accidentally clobber over ourself
        for ( std::size_t i = numConvert; i > 0; --i )
        {
            FROMPOD f = fromPodBuffer[i-1];
            if ( f < podMin )
            {
                f = podMin;
            }
            else if ( f > podMax )
            {
                f = podMax;
            }

            TOPOD t = static_cast< TOPOD >( f );
            toPodBuffer[i-1] = t;
        }
    }

}

//-*****************************************************************************
void
ConvertData( Alembic::Util::PlainOldDataType fromPod,
             Alembic::Util::PlainOldDataType toPod,
             char * fromBuffer,
             void * toBuffer,
             std::size_t iSize )
{

    switch (fromPod)
    {
        case Util::kBooleanPOD:
        {
            switch (toPod)
            {
                case Util::kUint8POD:
                {
                    ConvertFromBool< Util::uint8_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kInt8POD:
                {
                    ConvertFromBool< Util::int8_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kUint16POD:
                {
                    ConvertFromBool< Util::uint16_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kInt16POD:
                {
                    ConvertFromBool< Util::int16_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kUint32POD:
                {
                    ConvertFromBool< Util::uint32_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kInt32POD:
                {
                    ConvertFromBool< Util::int32_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kUint64POD:
                {
                    ConvertFromBool< Util::uint64_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kInt64POD:
                {
                    ConvertFromBool< Util::int64_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kFloat16POD:
                {
                    ConvertFromBool< Util::float16_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kFloat32POD:
                {
                    ConvertFromBool< Util::float32_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kFloat64POD:
                {
                    ConvertFromBool< Util::float64_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                default:
                break;
            }
        }
        break;

        case Util::kUint8POD:
        {
            switch (toPod)
            {
                case Util::kBooleanPOD:
                {
                    ConvertToBool< Util::uint8_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kInt8POD:
                {
                    ConvertData< Util::uint8_t, Util::int8_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kUint16POD:
                {
                    ConvertData< Util::uint8_t, Util::uint16_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kInt16POD:
                {
                    ConvertData< Util::uint8_t, Util::int16_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kUint32POD:
                {
                    ConvertData< Util::uint8_t, Util::uint32_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kInt32POD:
                {
                    ConvertData< Util::uint8_t, Util::int32_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kUint64POD:
                {
                    ConvertData< Util::uint8_t, Util::uint64_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kInt64POD:
                {
                    ConvertData< Util::uint8_t, Util::int64_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kFloat16POD:
                {
                    ConvertData< Util::uint8_t, Util::float16_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kFloat32POD:
                {
                    ConvertData< Util::uint8_t, Util::float32_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kFloat64POD:
                {
                    ConvertData< Util::uint8_t, Util::float64_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                default:
                break;
            }
        }
        break;

        case Util::kInt8POD:
        {
            switch (toPod)
            {
                case Util::kBooleanPOD:
                {
                    ConvertToBool< Util::int8_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kUint8POD:
                {
                    ConvertData< Util::int8_t, Util::uint8_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kUint16POD:
                {
                    ConvertData< Util::int8_t, Util::uint16_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kInt16POD:
                {
                    ConvertData< Util::int8_t, Util::int16_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kUint32POD:
                {
                    ConvertData< Util::int8_t, Util::uint32_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kInt32POD:
                {
                    ConvertData< Util::int8_t, Util::int32_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kUint64POD:
                {
                    ConvertData< Util::int8_t, Util::uint64_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kInt64POD:
                {
                    ConvertData< Util::int8_t, Util::int64_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kFloat16POD:
                {
                    ConvertData< Util::int8_t, Util::float16_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kFloat32POD:
                {
                    ConvertData< Util::int8_t, Util::float32_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kFloat64POD:
                {
                    ConvertData< Util::int8_t, Util::float64_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                default:
                break;
            }
        }
        break;

        case Util::kUint16POD:
        {
            switch (toPod)
            {
                case Util::kBooleanPOD:
                {
                    ConvertToBool< Util::uint16_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kUint8POD:
                {
                    ConvertData< Util::uint16_t, Util::uint8_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kInt8POD:
                {
                    ConvertData< Util::uint16_t, Util::int8_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kInt16POD:
                {
                    ConvertData< Util::uint16_t, Util::int16_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kUint32POD:
                {
                    ConvertData< Util::uint16_t, Util::uint32_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kInt32POD:
                {
                    ConvertData< Util::uint16_t, Util::int32_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kUint64POD:
                {
                    ConvertData< Util::uint16_t, Util::uint64_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kInt64POD:
                {
                    ConvertData< Util::uint16_t, Util::int64_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kFloat16POD:
                {
                    ConvertData< Util::uint16_t, Util::float16_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kFloat32POD:
                {
                    ConvertData< Util::uint16_t, Util::float32_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kFloat64POD:
                {
                    ConvertData< Util::uint16_t, Util::float64_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                default:
                break;
            }
        }
        break;

        case Util::kInt16POD:
        {
            switch (toPod)
            {
                case Util::kBooleanPOD:
                {
                    ConvertToBool< Util::int16_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kUint8POD:
                {
                    ConvertData< Util::int16_t, Util::uint8_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kInt8POD:
                {
                    ConvertData< Util::int16_t, Util::int8_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kUint16POD:
                {
                    ConvertData< Util::int16_t, Util::uint16_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kUint32POD:
                {
                    ConvertData< Util::int16_t, Util::uint32_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kInt32POD:
                {
                    ConvertData< Util::int16_t, Util::int32_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kUint64POD:
                {
                    ConvertData< Util::int16_t, Util::uint64_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kInt64POD:
                {
                    ConvertData< Util::int16_t, Util::int64_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kFloat16POD:
                {
                    ConvertData< Util::int16_t, Util::float16_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kFloat32POD:
                {
                    ConvertData< Util::int16_t, Util::float32_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kFloat64POD:
                {
                    ConvertData< Util::int16_t, Util::float64_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                default:
                break;
            }
        }
        break;

        case Util::kUint32POD:
        {
            switch (toPod)
            {
                case Util::kBooleanPOD:
                {
                    ConvertToBool< Util::uint32_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kUint8POD:
                {
                    ConvertData< Util::uint32_t, Util::uint8_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kInt8POD:
                {
                    ConvertData< Util::uint32_t, Util::int8_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kUint16POD:
                {
                    ConvertData< Util::uint32_t, Util::uint16_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kInt16POD:
                {
                    ConvertData< Util::uint32_t, Util::int16_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kInt32POD:
                {
                    ConvertData< Util::uint32_t, Util::int32_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kUint64POD:
                {
                    ConvertData< Util::uint32_t, Util::uint64_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kInt64POD:
                {
                    ConvertData< Util::uint32_t, Util::int64_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kFloat16POD:
                {
                    ConvertData< Util::uint32_t, Util::float16_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kFloat32POD:
                {
                    ConvertData< Util::uint32_t, Util::float32_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kFloat64POD:
                {
                    ConvertData< Util::uint32_t, Util::float64_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                default:
                break;
            }
        }
        break;

        case Util::kInt32POD:
        {
            switch (toPod)
            {
                case Util::kBooleanPOD:
                {
                    ConvertToBool< Util::int32_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kUint8POD:
                {
                    ConvertData< Util::int32_t, Util::uint8_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kInt8POD:
                {
                    ConvertData< Util::int32_t, Util::int8_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kUint16POD:
                {
                    ConvertData< Util::int32_t, Util::uint16_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kInt16POD:
                {
                    ConvertData< Util::int32_t, Util::int16_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kUint32POD:
                {
                    ConvertData< Util::int32_t, Util::uint32_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kUint64POD:
                {
                    ConvertData< Util::int32_t, Util::uint64_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kInt64POD:
                {
                    ConvertData< Util::int32_t, Util::int64_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kFloat16POD:
                {
                    ConvertData< Util::int32_t, Util::float16_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kFloat32POD:
                {
                    ConvertData< Util::int32_t, Util::float32_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kFloat64POD:
                {
                    ConvertData< Util::int32_t, Util::float64_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                default:
                break;
            }
        }
        break;

        case Util::kUint64POD:
        {
            switch (toPod)
            {
                case Util::kBooleanPOD:
                {
                    ConvertToBool< Util::uint64_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kUint8POD:
                {
                    ConvertData< Util::uint64_t, Util::uint8_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kInt8POD:
                {
                    ConvertData< Util::uint64_t, Util::int8_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kUint16POD:
                {
                    ConvertData< Util::uint64_t, Util::uint16_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kInt16POD:
                {
                    ConvertData< Util::uint64_t, Util::int16_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kUint32POD:
                {
                    ConvertData< Util::uint64_t, Util::uint32_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kInt32POD:
                {
                    ConvertData< Util::uint64_t, Util::int32_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kInt64POD:
                {
                    ConvertData< Util::uint64_t, Util::int64_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kFloat16POD:
                {
                    ConvertData< Util::uint64_t, Util::float16_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kFloat32POD:
                {
                    ConvertData< Util::uint64_t, Util::float32_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kFloat64POD:
                {
                    ConvertData< Util::uint64_t, Util::float64_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                default:
                break;
            }
        }
        break;

        case Util::kInt64POD:
        {
            switch (toPod)
            {
                case Util::kBooleanPOD:
                {
                    ConvertToBool<  Util::int64_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kUint8POD:
                {
                    ConvertData< Util::int64_t, Util::uint8_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kInt8POD:
                {
                    ConvertData< Util::int64_t, Util::int8_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kUint16POD:
                {
                    ConvertData< Util::int64_t, Util::uint16_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kInt16POD:
                {
                    ConvertData< Util::int64_t, Util::int16_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kUint32POD:
                {
                    ConvertData< Util::int64_t, Util::uint32_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kInt32POD:
                {
                    ConvertData< Util::int64_t, Util::int32_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kUint64POD:
                {
                    ConvertData< Util::int64_t, Util::uint64_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kFloat16POD:
                {
                    ConvertData< Util::int64_t, Util::float16_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kFloat32POD:
                {
                    ConvertData< Util::int64_t, Util::float32_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kFloat64POD:
                {
                    ConvertData< Util::int64_t, Util::float64_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                default:
                break;
            }
        }
        break;

        case Util::kFloat16POD:
        {
            switch (toPod)
            {
                case Util::kBooleanPOD:
                {
                    ConvertToBool< Util::float16_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kUint8POD:
                {
                    ConvertData< Util::float16_t, Util::uint8_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kInt8POD:
                {
                    ConvertData< Util::float16_t, Util::int8_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kUint16POD:
                {
                    ConvertData< Util::float16_t, Util::uint16_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kInt16POD:
                {
                    ConvertData< Util::float16_t, Util::int16_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kUint32POD:
                {
                    ConvertData< Util::float16_t, Util::uint32_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kInt32POD:
                {
                    ConvertData< Util::float16_t, Util::int32_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kUint64POD:
                {
                    ConvertData< Util::float16_t, Util::uint64_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kInt64POD:
                {
                    ConvertData< Util::float16_t, Util::int64_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kFloat32POD:
                {
                    ConvertData< Util::float16_t, Util::float32_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kFloat64POD:
                {
                    ConvertData< Util::float16_t, Util::float64_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                default:
                break;
            }
        }
        break;

        case Util::kFloat32POD:
        {
            switch (toPod)
            {
                case Util::kBooleanPOD:
                {
                    ConvertToBool< Util::float32_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kUint8POD:
                {
                    ConvertData< Util::float32_t, Util::uint8_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kInt8POD:
                {
                    ConvertData< Util::float32_t, Util::int8_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kUint16POD:
                {
                    ConvertData< Util::float32_t, Util::uint16_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kInt16POD:
                {
                    ConvertData< Util::float32_t, Util::int16_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kUint32POD:
                {
                    ConvertData< Util::float32_t, Util::uint32_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kInt32POD:
                {
                    ConvertData< Util::float32_t, Util::int32_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kUint64POD:
                {
                    ConvertData< Util::float32_t, Util::uint64_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kInt64POD:
                {
                    ConvertData< Util::float32_t, Util::int64_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kFloat16POD:
                {
                    ConvertData< Util::float32_t, Util::float16_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kFloat64POD:
                {
                    ConvertData< Util::float32_t, Util::float64_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                default:
                break;
            }
        }
        break;

        case Util::kFloat64POD:
        {
            switch (toPod)
            {
                case Util::kBooleanPOD:
                {
                    ConvertToBool< Util::float64_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kUint8POD:
                {
                    ConvertData< Util::float64_t, Util::uint8_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kInt8POD:
                {
                    ConvertData< Util::float64_t, Util::int8_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kUint16POD:
                {
                    ConvertData< Util::float64_t, Util::uint16_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kInt16POD:
                {
                    ConvertData< Util::float64_t, Util::int16_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kUint32POD:
                {
                    ConvertData< Util::float64_t, Util::uint32_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kInt32POD:
                {
                    ConvertData< Util::float64_t, Util::int32_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kUint64POD:
                {
                    ConvertData< Util::float64_t, Util::uint64_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kInt64POD:
                {
                    ConvertData< Util::float64_t, Util::int64_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kFloat16POD:
                {
                    ConvertData< Util::float64_t, Util::float16_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                case Util::kFloat32POD:
                {
                    ConvertData< Util::float64_t, Util::float32_t >(
                        fromBuffer, toBuffer, iSize );
                }
                break;

                default:
                break;
            }
        }
        break;

        default:
        break;
    }
}

//-*****************************************************************************
void
ReadData( void * iIntoLocation,
          Ogawa::IDataPtr iData,
          size_t iThreadId,
          const AbcA::DataType &iDataType,
          Util::PlainOldDataType iAsPod)
{
    Alembic::Util::PlainOldDataType curPod = iDataType.getPod();
    ABCA_ASSERT( ( iAsPod == curPod ) || (
        iAsPod != Alembic::Util::kStringPOD &&
        iAsPod != Alembic::Util::kWstringPOD &&
        curPod != Alembic::Util::kStringPOD &&
        curPod != Alembic::Util::kWstringPOD ),
        "Cannot convert the data to or from a string, or wstring." );

    if ( !iData )
    {
        ABCA_THROW("ReadData invalid: Null IDataPtr.");
        return;
    }
    std::size_t dataSize = iData->getSize();

    if ( dataSize <= 0 )
    {
        ABCA_ASSERT( dataSize == 0,
            "Incorrect data, expected to be empty or to have a key and data");
        return;
    }

    if ( curPod == Alembic::Util::kStringPOD )
    {
        if ( dataSize <= 0 )
        {
            return;
        }

        std::string * strPtr =
            reinterpret_cast< std::string * > ( iIntoLocation );

        std::size_t numChars = dataSize;
        char * buf = new char[ numChars ];
        iData->read( numChars, buf, 0, iThreadId );

        std::size_t startStr = 0;
        std::size_t strPos = 0;

        for ( std::size_t i = 0; i < numChars; ++i )
        {
            if ( buf[i] == 0 )
            {
                strPtr[strPos] = buf + startStr;
                startStr = i + 1;
                strPos ++;
            }
        }

        delete [] buf;
    }
    else if ( curPod == Alembic::Util::kWstringPOD )
    {
        if ( dataSize <= 0 )
        {
            return;
        }

        std::wstring * wstrPtr =
            reinterpret_cast< std::wstring * > ( iIntoLocation );

        std::size_t numChars = ( dataSize - 0 ) / 4;
        Util::uint32_t * buf = new Util::uint32_t[ numChars ];
        iData->read( dataSize, buf, 0, iThreadId );

        std::size_t strPos = 0;

        // push these one at a time until we can figure out how to cast like
        // strings above
        for ( std::size_t i = 0; i < numChars; ++i )
        {
            std::wstring & wstr = wstrPtr[strPos];
            if ( buf[i] == 0 )
            {
                strPos ++;
            }
            else
            {
                wstr.push_back( buf[i] );
            }
        }

        delete [] buf;
    }
    else if ( iAsPod == curPod )
    {
        // don't read the key
        iData->read( dataSize, iIntoLocation, 0, iThreadId );
    }
    else if ( PODNumBytes( curPod ) <= PODNumBytes( iAsPod ) )
    {
        // - 16 to skip key
        std::size_t numBytes = dataSize;

        iData->read( numBytes, iIntoLocation, 0, iThreadId );

        char * buf = static_cast< char * >( iIntoLocation );
        ConvertData( curPod, iAsPod, buf, iIntoLocation, numBytes );

    }
    else if ( PODNumBytes( curPod ) > PODNumBytes( iAsPod ) )
    {
        // - 16 to skip key
        std::size_t numBytes = dataSize;

        // read into a temporary buffer and cast them one at a time
        char * buf = new char[ numBytes ];
        iData->read( numBytes, buf, 0, iThreadId );

        ConvertData( curPod, iAsPod, buf, iIntoLocation, numBytes );

        delete [] buf;
    }

}

//-*****************************************************************************
void
ReadArrayData( void * iIntoLocation,
          Ogawa::IDataPtr iData,
          size_t iThreadId,
          const AbcA::DataType &iDataType,
          Util::PlainOldDataType iAsPod)
{
    Alembic::Util::PlainOldDataType curPod = iDataType.getPod();
    ABCA_ASSERT( ( iAsPod == curPod ) || (
        iAsPod != Alembic::Util::kStringPOD &&
        iAsPod != Alembic::Util::kWstringPOD &&
        curPod != Alembic::Util::kStringPOD &&
        curPod != Alembic::Util::kWstringPOD ),
        "Cannot convert the data to or from a string, or wstring." );

    if ( !iData )
    {
        ABCA_THROW("ReadData invalid: Null IDataPtr.");
        return;
    }

    std::size_t dataSize = iData->getSize();

    if ( dataSize < 8 )
    {
        ABCA_ASSERT( dataSize == 0,
            "Incorrect data, expected to be empty or to have a key and data");
        return;
    }

    // the decompressed data size
    std::size_t decompressedDataSize = 0;
    iData->read(8, &decompressedDataSize, 0, 0);

    if ( curPod == Alembic::Util::kStringPOD )
    {
        if ( dataSize <= 8 )
        {
            return;
        }

        std::string * strPtr =
            reinterpret_cast< std::string * > ( iIntoLocation );

        std::size_t numChars = dataSize - 8;
        char * buf = new char[ numChars ];
        iData->read( numChars, buf, 8, iThreadId );

        // decompress the zstd data
        char * newBuf = new char[ decompressedDataSize ];
        size_t result = ZSTD_decompress(newBuf, decompressedDataSize, buf, numChars);

        std::size_t startStr = 0;
        std::size_t strPos = 0;

        for ( std::size_t i = 0; i < numChars; ++i )
        {
            if ( newBuf[i] == 0 )
            {
                strPtr[strPos] = newBuf + startStr;
                startStr = i + 1;
                strPos ++;
            }
        }

        delete [] buf;
        delete [] newBuf;
    }
    else if ( curPod == Alembic::Util::kWstringPOD )
    {
        if ( dataSize <= 8 )
        {
            return;
        }

        std::wstring * wstrPtr =
            reinterpret_cast< std::wstring * > ( iIntoLocation );

        std::size_t numChars = ( dataSize - 8 ) / 4;
        Util::uint32_t * buf = new Util::uint32_t[ numChars ];
        iData->read( dataSize - 8, buf, 8, iThreadId );

        // decompress the zstd data
        Util::uint32_t * newBuf = new Util::uint32_t[ decompressedDataSize / 4 ];
        size_t result = ZSTD_decompress(newBuf, decompressedDataSize / 4, buf, numChars);

        std::size_t strPos = 0;

        // push these one at a time until we can figure out how to cast like
        // strings above
        for ( std::size_t i = 0; i < numChars; ++i )
        {
            std::wstring & wstr = wstrPtr[strPos];
            if ( newBuf[i] == 0 )
            {
                strPos ++;
            }
            else
            {
                wstr.push_back( newBuf[i] );
            }
        }

        delete [] buf;
        delete [] newBuf;
    }
    else if ( iAsPod == curPod )
    {
        // don't read the key
        char * buf = new char[ dataSize - 8 ];
        iData->read( dataSize - 8, buf, 8, iThreadId );
        size_t result = ZSTD_decompress(iIntoLocation, decompressedDataSize, buf, dataSize - 8);

        delete [] buf;
    }
    else if ( PODNumBytes( curPod ) <= PODNumBytes( iAsPod ) )
    {
        std::size_t numBytes = dataSize - 8;

        char * compressedBuf = new char[ numBytes ];
        iData->read( numBytes, compressedBuf, 8, iThreadId );
        size_t result = ZSTD_decompress(iIntoLocation, decompressedDataSize, compressedBuf, numBytes);

        char * buf = static_cast< char * >( iIntoLocation );
        ConvertData( curPod, iAsPod, buf, iIntoLocation, numBytes );

        delete [] compressedBuf;
    }
    else if ( PODNumBytes( curPod ) > PODNumBytes( iAsPod ) )
    {
        std::size_t numBytes = dataSize - 8;

        char * compressedBuf = new char[ numBytes ];
        iData->read( numBytes, compressedBuf, 8, iThreadId );

        char * buf = new char[ decompressedDataSize ];
        size_t result = ZSTD_decompress(buf, decompressedDataSize, compressedBuf, numBytes);
        ConvertData( curPod, iAsPod, buf, iIntoLocation, numBytes );

        delete [] buf;
        delete [] compressedBuf;
    }

}

//-*****************************************************************************
void
ReadArraySample( Ogawa::IDataPtr iDims,
                 Ogawa::IDataPtr iData,
                 size_t iThreadId,
                 const AbcA::DataType &iDataType,
                 AbcA::ArraySamplePtr &oSample )
{
    // get our dimensions
    Util::Dimensions dims;
    ReadTDRDimensions( iDims, iData, iThreadId, iDataType, dims );

    oSample = AbcA::AllocateArraySample( iDataType, dims );

    ReadArrayData( const_cast<void*>( oSample->getData() ), iData,
        iThreadId, iDataType, iDataType.getPod() );
}

//-*****************************************************************************
template < typename POD >
static inline POD DerefUnaligned(const void* iData)
{
    // on some platforms, dereferencing an unaligned pointer causes a crash. so, copy byte by byte.
    POD ret;
    memcpy(&ret, iData, sizeof(POD));
    return ret;
}

//-*****************************************************************************
void
ReadTimeSamplesAndMax( Ogawa::IDataPtr iData,
                       std::vector <  AbcA::TimeSamplingPtr > & oTimeSamples,
                       std::vector <  AbcA::index_t > & oMaxSamples )
{
    std::vector< char > buf( iData->getSize() );
    if ( buf.empty() )
    {
        return;
    }

    iData->read( iData->getSize(), &( buf.front() ), 0, 0 );
    std::size_t pos = 0;
    std::size_t bufSize = buf.size();
    while ( pos < bufSize )
    {
        // make sure maxSample, tpc, and numSamples
        // don't go beyond our buffer
        if ( pos + 8 + sizeof( chrono_t ) > bufSize)
        {
            ABCA_THROW("Read invalid: TimeSamples info.");
        }

        Util::uint32_t maxSample = DerefUnaligned<Util::uint32_t>(&buf[pos]);
        pos += 4;

        oMaxSamples.push_back( maxSample );

        chrono_t tpc = DerefUnaligned<chrono_t>(&buf[pos]);
        pos += sizeof( chrono_t );

        Util::uint32_t numSamples = DerefUnaligned<Util::uint32_t>(&buf[pos]);
        pos += 4;

        // make sure our numSamples don't go beyond the buffer and that we
        // have at least 1 of them
        if ( numSamples < 1 || pos + sizeof( chrono_t ) * numSamples > bufSize)
        {
            ABCA_THROW("Read invalid: TimeSamples sample times.");
        }

        std::vector< chrono_t > sampleTimes( numSamples );
        memcpy( &( sampleTimes.front() ), &buf[pos],
                sizeof( chrono_t ) * numSamples );
        pos += sizeof( chrono_t ) * numSamples;

        AbcA::TimeSamplingType::AcyclicFlag acf =
            AbcA::TimeSamplingType::kAcyclic;

        AbcA::TimeSamplingType tst( acf );
        if ( tpc != AbcA::TimeSamplingType::AcyclicTimePerCycle() )
        {
            tst = AbcA::TimeSamplingType( numSamples, tpc );
        }

        AbcA::TimeSamplingPtr tptr(
            new AbcA::TimeSampling( tst, sampleTimes ) );

        oTimeSamples.push_back( tptr );
    }
}

//-*****************************************************************************
void
ReadObjectHeaders( Ogawa::IGroupPtr iGroup,
                   size_t iIndex,
                   size_t iThreadId,
                   const std::string & iParentName,
                   const std::vector< AbcA::MetaData > & iMetaDataVec,
                   std::vector< ObjectHeaderPtr > & oHeaders )
{
    Ogawa::IDataPtr data = iGroup->getData( iIndex, iThreadId );
    ABCA_ASSERT( data, "ReadObjectHeaders Invalid data at index " << iIndex );

    std::vector< char > buf( data->getSize() );
    if ( buf.empty() )
    {
        return;
    }

    std::size_t bufSize = buf.size();
    data->read( bufSize, &( buf.front() ), 0, iThreadId );
    std::size_t pos = 0;
    while ( pos < bufSize )
    {
        if (pos + 4 > bufSize)
        {
            ABCA_THROW("Read invalid: Object Headers name size.");
        }

        Util::uint32_t nameSize = DerefUnaligned<Util::uint32_t>(&buf[pos]);
        pos += 4;

        if (nameSize == 0 || pos + nameSize + 1 > bufSize)
        {
            ABCA_THROW("Read invalid: Object Headers name and MetaData index.");
        }

        std::string name( &buf[pos], nameSize );
        pos += nameSize;

        Util::uint8_t metaDataIndex = buf[pos++];

        ObjectHeaderPtr objPtr( new AbcA::ObjectHeader() );
        objPtr->setName( name );
        objPtr->setFullName( iParentName + "/" + name );

        if ( metaDataIndex == 0xff )
        {
            if (pos + 4 > bufSize)
            {
                ABCA_THROW("Read invalid: Object Headers MetaData size.");
            }

            Util::uint32_t metaDataSize = DerefUnaligned<Util::uint32_t>(&buf[pos]);
            pos += 4;

            if (pos + metaDataSize > bufSize)
            {
                ABCA_THROW("Read invalid: Object Headers MetaData string.");
            }

            std::string metaData( &buf[pos], metaDataSize );
            pos += metaDataSize;

            objPtr->getMetaData().deserialize( metaData );
        }
        else if ( metaDataIndex < iMetaDataVec.size() )
        {
            objPtr->getMetaData() = iMetaDataVec[metaDataIndex];
        }
        else
        {
            ABCA_THROW("Read invalid: Object Headers MetaData index.");
        }


        oHeaders.push_back( objPtr );
    }
}

//-*****************************************************************************
Util::uint32_t GetUint32WithHint(const std::vector< char > & iBuf,
                           std::size_t iBufSize,
                           Util::uint32_t iSizeHint,
                           std::size_t & ioPos)
{
    Util::uint32_t retVal = 0;

    if ( iSizeHint == 0 && ioPos + 1 <= iBufSize )
    {
        retVal = ( Util::uint32_t ) ( ( Util::uint8_t ) iBuf[ioPos] );
        ioPos ++;
    }
    else if ( iSizeHint == 1 && ioPos + 2 <= iBufSize )
    {
        retVal = ( Util::uint32_t )( *( (Util::uint16_t *)( &iBuf[ioPos] ) ) );
        ioPos += 2;
    }
    else if ( iSizeHint == 2 && ioPos + 4 <= iBufSize )
    {
        retVal = *( ( Util::uint32_t * )( &iBuf[ioPos] ) );
        ioPos += 4;
    }
    else
    {
        ABCA_THROW("Read invalid: Property Header bad uint32 hint.");
    }

    return retVal;
}

//-*****************************************************************************
void
ReadPropertyHeaders( Ogawa::IGroupPtr iGroup,
                     size_t iIndex,
                     size_t iThreadId,
                     AbcA::ArchiveReader & iArchive,
                     const std::vector< AbcA::MetaData > & iMetaDataVec,
                     PropertyHeaderPtrs & oHeaders )
{

    // Our bitmasks look like this:
    //
    // Property Type mask (Scalar, Array, or Compound) 0x0003
    // 0000 0000 0000 0000 0000 0000 0000 0011
    //
    // Our Pod type mask 0x003c
    // 0000 0000 0000 0000 0000 0000 0011 1100
    //
    // Has a time sampling index mask 0x0040
    // 0000 0000 0000 0000 0000 0000 0100 0000
    //
    // no repeats mask 0x0080
    // 0000 0000 0000 0000 0000 0000 1000 0000
    //
    // Extent value mask 0xff00
    // 0000 0000 0000 0000 1111 1111 0000 0000
    //
    // Our bitmasks look like this:
    //
    // Property Type mask (Scalar, Array, or Compound) 0x0003
    // 0000 0000 0000 0000 0000 0000 0000 0011
    //
    // Byte size hint for first/last non repeated sample 0x000c
    // 0000 0000 0000 0000 0000 0000 0000 1100
    //
    // Our Pod type mask 0x00f0
    // 0000 0000 0000 0000 0000 0000 1111 0000
    //
    // Has a time sampling index mask 0x0100
    // 0000 0000 0000 0000 0000 0001 0000 0000
    //
    // Whether the first/last index exists mask 0x200
    // 0000 0000 0000 0000 0000 0010 0000 0000
    //
    // Whether the size of the data changes from sample to sample mask 0x400
    // 0000 0000 0000 0000 0000 0100 0000 0000
    //
    // Mask for whether the data is the same over all samples 0x800
    // 0000 0000 0000 0000 0000 1000 0000 0000
    //
    // Extent value mask 0xff000
    // 0000 0000 0000 1111 1111 0000 0000 0000
    //
    // Meta data index mask 0xff00000
    // 0000 1111 1111 0000 0000 0000 0000 0000

    Ogawa::IDataPtr data = iGroup->getData( iIndex, iThreadId );
    ABCA_ASSERT( data, "ReadObjectHeaders Invalid data at index " << iIndex );

    if ( data->getSize() == 0 )
    {
        return;
    }

    std::vector< char > buf( data->getSize() );
    data->read( data->getSize(), &( buf.front() ), 0, iThreadId );
    std::size_t pos = 0;
    std::size_t bufSize = buf.size();
    while ( pos < bufSize )
    {
        PropertyHeaderPtr header( new PropertyHeaderAndFriends() );

        if (pos + 4 > bufSize)
        {
            ABCA_THROW("Read invalid: Property header start.");
        }

        // first 4 bytes is always info
        Util::uint32_t info = DerefUnaligned<Util::uint32_t>(&buf[pos]);
        pos += 4;

        Util::uint32_t ptype = info & 0x0003;
        header->isScalarLike = ptype & 1;
        if ( ptype == 0 )
        {
            header->header.setPropertyType( AbcA::kCompoundProperty );
        }
        else if ( ptype == 1 )
        {
            header->header.setPropertyType( AbcA::kScalarProperty );
        }
        else
        {
            header->header.setPropertyType( AbcA::kArrayProperty );
        }

        Util::uint32_t sizeHint = ( info & 0x000c ) >> 2;

        // if we aren't a compound we may need to do a bunch of other work
        if ( !header->header.isCompound() )
        {
            // Read the pod type out of bits 4-7
            char podt = ( char )( ( info &  0x00f0 ) >> 4 );
            if ( podt != ( char )Alembic::Util::kBooleanPOD &&
                 podt != ( char )Alembic::Util::kUint8POD &&
                 podt != ( char )Alembic::Util::kInt8POD &&
                 podt != ( char )Alembic::Util::kUint16POD &&
                 podt != ( char )Alembic::Util::kInt16POD &&
                 podt != ( char )Alembic::Util::kUint32POD &&
                 podt != ( char )Alembic::Util::kInt32POD &&
                 podt != ( char )Alembic::Util::kUint64POD &&
                 podt != ( char )Alembic::Util::kInt64POD &&
                 podt != ( char )Alembic::Util::kFloat16POD &&
                 podt != ( char )Alembic::Util::kFloat32POD &&
                 podt != ( char )Alembic::Util::kFloat64POD &&
                 podt != ( char )Alembic::Util::kStringPOD &&
                 podt != ( char )Alembic::Util::kWstringPOD )
            {
                ABCA_THROW(
                    "Read invalid POD type: " << ( Util::int32_t )podt );
            }

            Util::uint8_t extent = ( info & 0xff000 ) >> 12;
            header->header.setDataType( AbcA::DataType(
                ( Util::PlainOldDataType ) podt, extent ) );

            header->isHomogenous = ( info & 0x400 ) != 0;

            header->nextSampleIndex = GetUint32WithHint( buf, bufSize, sizeHint, pos );

            if ( ( info & 0x0200 ) != 0 )
            {
                header->firstChangedIndex =
                    GetUint32WithHint( buf, bufSize, sizeHint, pos );

                header->lastChangedIndex =
                    GetUint32WithHint( buf, bufSize, sizeHint, pos );
            }
            else if ( ( info & 0x800 ) != 0 )
            {
                header->firstChangedIndex = 0;
                header->lastChangedIndex = 0;
            }
            else
            {
                header->firstChangedIndex = 1;
                header->lastChangedIndex = header->nextSampleIndex - 1;
            }

            if ( ( info & 0x0100 ) != 0 )
            {
                header->timeSamplingIndex =
                    GetUint32WithHint( buf, bufSize, sizeHint, pos );

                header->header.setTimeSampling(
                    iArchive.getTimeSampling( header->timeSamplingIndex ) );
            }
            else
            {
                header->header.setTimeSampling( iArchive.getTimeSampling( 0 ) );
            }
        }

        Util::uint32_t nameSize = GetUint32WithHint( buf, bufSize, sizeHint, pos );
        if ( nameSize == 0 || pos + nameSize > bufSize )
        {
            ABCA_THROW("Read invalid: Property Headers name.");
        }

        std::string name( &buf[pos], nameSize );
        header->header.setName( name );
        pos += nameSize;

        Util::uint32_t metaDataIndex = ( info & 0xff00000 ) >> 20;

        if ( metaDataIndex == 0xff )
        {
            Util::uint32_t metaDataSize =
                GetUint32WithHint( buf, bufSize, sizeHint, pos );

            if (pos + metaDataSize > bufSize)
            {
                ABCA_THROW("Read invalid: Property Header MetaData string.");
            }
            // found empty metadata
            else if (pos == bufSize)
            {
                pos += metaDataSize;
                AbcA::MetaData md;
                header->header.setMetaData( md );
            }
            else
            {
                std::string metaData( &buf[pos], metaDataSize );
                pos += metaDataSize;

                AbcA::MetaData md;
                md.deserialize( metaData );
                header->header.setMetaData( md );
            }
        }
        else if (metaDataIndex < iMetaDataVec.size())
        {
            header->header.setMetaData( iMetaDataVec[metaDataIndex] );
        }
        else
        {
            ABCA_THROW("Read invalid: Property Header MetaData index.");
        }

        oHeaders.push_back( header );

    }
}

void
ReadIndexedMetaData( Ogawa::IDataPtr iData,
                     std::vector< AbcA::MetaData > & oMetaDataVec )
{
    // add the default empty meta data
    oMetaDataVec.push_back( AbcA::MetaData() );

    // we only index small meta data, under 256 bytes each
    // and we only allow 256 indices
    if ( iData->getSize() > 65536 )
    {
        ABCA_THROW("Read invalid: Indexed MetaData buffer unexpectedly big.");
    }

    std::vector< char > buf( iData->getSize() );

    if ( buf.empty() )
    {
        return;
    }

    // read as part of opening the archive so threadid 0 is ok
    iData->read( iData->getSize(), &( buf.front() ), 0, 0 );
    std::size_t pos = 0;
    std::size_t bufSize = buf.size();
    while ( pos < bufSize )
    {
        if (pos + 1 > bufSize)
        {
            ABCA_THROW("Read invalid: Indexed MetaData string size.");
        }

        // these are all small (less than 256 byte) meta data strings
        Util::uint8_t metaDataSize = buf[pos++];

        if (pos + metaDataSize > bufSize)
        {
            ABCA_THROW("Read invalid: Indexed MetaData string.");
        }
        else if (pos == bufSize)
        {
            pos += metaDataSize;
            AbcA::MetaData md;
            oMetaDataVec.push_back( md );
        }
        else
        {
            std::string metaData( &buf[pos], metaDataSize );
            pos += metaDataSize;
            AbcA::MetaData md;
            md.deserialize( metaData );
            oMetaDataVec.push_back( md );
        }
    }
}

} // End namespace ALEMBIC_VERSION_NS
} // End namespace AbcCoreOgawa
} // End namespace Alembic
