#include "FWCore/Framework/interface/ESHandle.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "MagneticField/Records/interface/IdealMagneticFieldRecord.h" 
#include "Alignment/ReferenceTrajectories/interface/TrajectoryFactoryPlugin.h"
// Do not include .h from plugin directory, but locally:
#include "ReferenceTrajectoryFactory.h"
#include "Alignment/ReferenceTrajectories/interface/ReferenceTrajectory.h" 


ReferenceTrajectoryFactory::ReferenceTrajectoryFactory( const edm::ParameterSet & config ) :
  TrajectoryFactoryBase( config )
{
  theMass = config.getParameter< double >( "ParticleMass" );
}

 
ReferenceTrajectoryFactory::~ReferenceTrajectoryFactory( void ) {}


const ReferenceTrajectoryFactory::ReferenceTrajectoryCollection
ReferenceTrajectoryFactory::trajectories( const edm::EventSetup & setup,
					  const ConstTrajTrackPairCollection & tracks ) const
{
  ReferenceTrajectoryCollection trajectories;

  edm::ESHandle< MagneticField > magneticField;
  setup.get< IdealMagneticFieldRecord >().get( magneticField );

  ConstTrajTrackPairCollection::const_iterator itTracks = tracks.begin();

  while ( itTracks != tracks.end() )
  { 
    TrajectoryInput input = this->innermostStateAndRecHits( *itTracks );
    // set the flag for reversing the RecHits to false, since they are already in the correct order.
    trajectories.push_back( ReferenceTrajectoryPtr( new ReferenceTrajectory( input.first, input.second, 
									     false, magneticField.product(),
									     theMaterialEffects, theMass ) ) );
    ++itTracks;
  }

  return trajectories;
}

const ReferenceTrajectoryFactory::ReferenceTrajectoryCollection
ReferenceTrajectoryFactory::trajectories( const edm::EventSetup & setup,
					  const ConstTrajTrackPairCollection& tracks,
					  const ExternalPredictionCollection& external ) const
{
  ReferenceTrajectoryCollection trajectories;

  if ( tracks.size() != external.size() )
  {
    edm::LogInfo("Alignment") << "@SUB=ReferenceTrajectoryFactory::trajectories"
			      << "Inconsistent input:" << std::endl
			      << "\tnumber of tracks = " << tracks.size()
			      << "\tnumber of external predictions = " << external.size() << std::endl;
    return trajectories;
  }

  edm::ESHandle< MagneticField > magneticField;
  setup.get< IdealMagneticFieldRecord >().get( magneticField );

  ConstTrajTrackPairCollection::const_iterator itTracks = tracks.begin();
  ExternalPredictionCollection::const_iterator itExternal = external.begin();

  while ( itTracks != tracks.end() )
  {
    TrajectoryInput input = innermostStateAndRecHits( *itTracks  );

    if ( (*itExternal).isValid() )
    {
      // set the flag for reversing the RecHits to false, since they are already in the correct order.
      ReferenceTrajectoryPtr refTraj( new ReferenceTrajectory( *itExternal, input.second,
							       false, magneticField.product(),
							       theMaterialEffects, theMass ) );

      AlgebraicSymMatrix externalParamErrors( asHepMatrix<5>( (*itExternal).localError().matrix() ) );
      refTraj->setParameterErrors( externalParamErrors );
      trajectories.push_back( refTraj );
    }
    else
    {
      trajectories.push_back( ReferenceTrajectoryPtr( new ReferenceTrajectory( input.first, input.second, 
									       false, magneticField.product(),
									       theMaterialEffects, theMass ) ) );
    }

    ++itTracks;
    ++itExternal;
  }

  return trajectories;
}


DEFINE_EDM_PLUGIN( TrajectoryFactoryPlugin, ReferenceTrajectoryFactory, "ReferenceTrajectoryFactory" );
