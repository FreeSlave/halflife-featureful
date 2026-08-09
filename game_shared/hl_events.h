#pragma once
#ifndef HL_EVENTS_H
#define HL_EVENTS_H

extern "C"
{
// HLDM
void EV_FireConfigurableWeapon( struct event_args_s *args );
void EV_FireGauss( struct event_args_s *args );
void EV_SpinGauss( struct event_args_s *args );
void EV_FireCrossbow2( struct event_args_s *args );
void EV_EgonFire( struct event_args_s *args );
void EV_EgonStop( struct event_args_s *args );
void EV_TripmineFire( struct event_args_s *args );
void EV_SnarkFire( struct event_args_s *args );

void EV_TrainPitchAdjust( struct event_args_s *args );
void EV_VehiclePitchAdjust( struct event_args_s *args );

void EV_Displacer( struct event_args_s *args );
void EV_MedkitFire( struct event_args_s *args );
void EV_BlowTorchIdle( struct event_args_s *args );
}

#endif
