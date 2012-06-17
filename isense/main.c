//==========================================================================================
//
//    File Name:      main.c
//    Description:    Illustrates use of the InterSense driver API
//    Created:        12/7/98
//    Copyright:      InterSense 2007 - All rights Reserved.
//
//    Comments:       This program illustrates the use of the functions
//                    defined in isense.h. 
//                    
//==========================================================================================

#include <stdio.h> 
#include <stdlib.h> 
#include <memory.h>
#include <string.h>

#if defined(_WIN32) || defined(WIN32) || defined(__WIN32__)
#include <conio.h>
#endif

// Needed to write the Unix equivalent of kbhit()
#if defined(UNIX)
#include <termios.h>
#include <unistd.h>
#endif

#include "isense.h"

#define ESC 0X1B

//==========================================================================================
const char *systemType( int Type ) 
{
    switch( Type ) 
    {
    case ISD_NONE:
        return "Unknown";
    case ISD_PRECISION_SERIES:
        return "IS Precision Series";
    case ISD_INTERTRAX_SERIES:
        return "InterTrax Series";
    }
    return "Unknown";
}


//==========================================================================================
const char *systemName( int Model ) 
{
    switch( Model ) 
    {
    case ISD_IS300:
        return "IS-300 Series";
    case ISD_IS600:
        return "IS-600 Series";
    case ISD_IS900:
        return "IS-900 Series";
    case ISD_INTERTRAX:
        return "InterTrax 30";
    case ISD_INTERTRAX_2:
        return "InterTrax2";
    case ISD_INTERTRAX_LS:
        return "InterTraxLS";
    case ISD_INTERTRAX_LC:
        return "InterTraxLC";
    case ISD_ICUBE2:
    case ISD_ICUBE2_PRO:
    case ISD_ICUBE2B_PRO:
        return "InertiaCube2";
    case ISD_ICUBE3:
        return "InertiaCube3";
    case ISD_IS1200:
        return "IS-1200 Series";
    }
    return "Unknown";
}


//==========================================================================================
//
//  Get and display tracker information
//
//==========================================================================================
void showTrackerStats( ISD_TRACKER_HANDLE handle, ISD_HARDWARE_INFO_TYPE *hwInfo )
{
    ISD_TRACKER_INFO_TYPE Tracker;
    ISD_STATION_INFO_TYPE Station;
    WORD i, numStations = 4;
    char buf[20];

    if( ISD_GetTrackerConfig( handle, &Tracker, TRUE ) )
    {
        printf( "\n\n********** InterSense Tracker Information ***********\n\n" );

        printf( "Type:     %s device on port %d\n", 
            systemType(Tracker.TrackerType), Tracker.Port );
        
        printf("Model:    %s\n",  
            hwInfo->Valid ?  hwInfo->ModelName : systemName(Tracker.TrackerModel) );

        switch( Tracker.TrackerModel ) 
        {
        case ISD_IS300:
        case ISD_IS1200:
            numStations = 4;
            break;
        case ISD_IS600:
        case ISD_IS900:
            numStations = ISD_MAX_STATIONS;
            break;
        default:
            numStations = 1;
            break;
        }
        
        printf( "\nStation\tTime\tState\tCube  Enhancement  Sensitivity  Compass  Prediction\n" );
        
        for(i = 1; i <= numStations; i++)
        {
            printf( "%d\t", i );
            
            if( ISD_GetStationConfig( handle, &Station, i, FALSE ))
            {
                sprintf(buf, "%d", Station.InertiaCube);
                
                printf( "%s\t%s\t%s\t   %u\t\t%u\t  %u\t  %u\n", 
                    Station.TimeStamped ? "ON" : "OFF", 
                    Station.State ? "ON" : "OFF", 
                    Station.InertiaCube == -1 ? "None" : buf, 
                    Station.Enhancement, 
                    Station.Sensitivity, 
                    Station.Compass, 
                    Station.Prediction );
            }
            else
            {
                printf("ISD_GetStationConfig failed\n");
                break;
            }
        }
        printf("\n");
    }
    else
    {
        printf("ISD_GetTrackerConfig failed\n");
    }
}


//==========================================================================================
//
//  Display Tracker data
//
//==========================================================================================
void showStationData( ISD_TRACKER_HANDLE             handle, 
                      ISD_TRACKER_INFO_TYPE         *Tracker,
                      ISD_STATION_INFO_TYPE         *Station,
                      ISD_STATION_DATA_TYPE         *data )
{
    // Get comm port statistics for display with tracker data 
    if( ISD_GetCommInfo( handle, Tracker ) )
    {
        printf( "%3.0fKb/s %d R/s ", 
            Tracker->KBitsPerSec, Tracker->RecordsPerSec );

        // display position only if system supports it 
        if( Tracker->TrackerModel == ISD_IS600 || 
            Tracker->TrackerModel == ISD_IS900 ||
            Tracker->TrackerModel == ISD_IS1200 )
        {
            printf( "[%d%%] (%6.2f,%6.2f,%6.2f)m ", (int)(data->TrackingStatus/2.55),
                data->Position[0], data->Position[1], data->Position[2] );
        }

        // all products can return orientation 
        if( Station->AngleFormat == ISD_QUATERNION )
        {
            printf( "%5.2f %5.2f %5.2f %5.2f ",
                data->Quaternion[0], data->Quaternion[1], 
                data->Quaternion[2], data->Quaternion[3] );
        }
        else // Euler angles
        {
            printf( "(%7.2f,%7.2f,%7.2f)deg ",
                data->Euler[0], data->Euler[1], data->Euler[2]);
        }

	printf( "%7.1fs ", data->TimeStamp );

        if( Station->GetAuxInputs ) 
        {
            printf("%d %d %d %d ", 
                (int) data->AuxInputs[0], 
                (int) data->AuxInputs[1], 
                (int) data->AuxInputs[2], 
                (int) data->AuxInputs[3]);
        }
        
        // if system is configured to read stylus or wand buttons 
        if( Station->GetInputs ) 
        {
            // Currently available products have at most 6 buttons,
            printf("%d%d%d%d%d%d ", 
                (int) data->ButtonState[0], 
                (int) data->ButtonState[1], 
                (int) data->ButtonState[2], 
                (int) data->ButtonState[3], 
                (int) data->ButtonState[4], 
                (int) data->ButtonState[5]);

            printf( "%d %d ", data->AnalogData[0], data->AnalogData[1] ); 
        }

        printf("\r");
        fflush(0);
    }
}

// Unix doesn't have kbhit, so a similar function is needed
// Similarly nothing equivalent to getch(), since getc/getchar() require an enter press
#if defined(UNIX)

// Check to make sure getchar() won't block
int unix_kbhit(void)
{
    int i, ch;
    fd_set fds;
    struct timeval tv;
    struct termios t_new, t_old;

    tcgetattr( STDIN_FILENO, &t_new );
    t_old = t_new;			// Copy settings
    t_new.c_lflag &= ~ICANON;  // Turn off echo and buffering
    tcsetattr( STDIN_FILENO, TCSANOW, &t_new );   // Apply settings
    
    FD_ZERO(&fds);                              // Clear the fd_set
    FD_SET(STDIN_FILENO, &fds);                 // Watch STDIN for input
    tv.tv_sec = tv.tv_usec = 0;                 // 0 second timeout value
    i = select(STDIN_FILENO+1, &fds, NULL, NULL, &tv);

    if (i != 0)
        ch = getchar();

    // Reload old settings
    tcsetattr( STDIN_FILENO, TCSANOW, &t_old );

    // If there is something typed so that getchar() won't block, return true, else return false
    if (i == -1) return (FALSE);                // Only returned on error
    if (i != 0) return (ch);    		// Returned if getchar() will be non-blocking

    return (FALSE);                             // Returned if getchar() would block
}
#endif

//==========================================================================================
//
// This simple main shows how to initialize and get data from InterSense trackers. 
//
//==========================================================================================
int main()
{
    ISD_TRACKER_HANDLE              handle;
    ISD_TRACKING_DATA_TYPE          data;
    ISD_STATION_INFO_TYPE           Station[ISD_MAX_STATIONS];
    ISD_TRACKER_INFO_TYPE           Tracker;
    ISD_HARDWARE_INFO_TYPE          hwInfo;
    ISD_STATION_HARDWARE_INFO_TYPE  stationHwInfo[ISD_MAX_STATIONS];

   
    WORD done = FALSE, station = 1;
    DWORD maxStations = 4;
    float lastTime; 
#if defined UNIX
    char ch_key;
#endif

    // Detect first tracker. If you have more than one InterSense device and
    // would like to have a specific tracker, connected to a known port, 
    // initialized first, then enter the port number instead of 0. Otherwise, 
    // tracker connected to the rs232 port with lower number is found first 

    handle = ISD_OpenTracker( (Hwnd) NULL, 0, FALSE, TRUE );
    
    // Check value of handle to see if tracker was located 
    if( handle < 1 )
    {
        printf( "Failed to detect InterSense tracking device\n" );
    }
    else
    {
        // Get tracker configuration info 
        ISD_GetTrackerConfig( handle, &Tracker, TRUE );
                
        memset((void *) &hwInfo, 0, sizeof(hwInfo));

        if( ISD_GetSystemHardwareInfo( handle, &hwInfo ) )
        {
            if( hwInfo.Valid )
            {
                maxStations = hwInfo.Capability.MaxStations;
            }
        }

        // Clear station configuration info to make sure GetAnalogData and other flags are FALSE 
        memset( (void *) Station, 0, sizeof(Station) );

        // General procedure for changing any setting is to first retrieve current 
        // configuration, make the change, and then apply it. Calling 
        // ISD_GetStationConfig is important because you only want to change 
        // some of the settings, leaving the rest unchanged. 
        
        if( Tracker.TrackerType == ISD_PRECISION_SERIES )
        {
            for( station = 1; station <= maxStations; station++ )
            {         
                // fill ISD_STATION_INFO_TYPE structure with current station configuration 
                if( !ISD_GetStationConfig( handle, 
                    &Station[station-1], station, TRUE ) ) break;
                
                if( !ISD_GetStationHardwareInfo( handle, 
                    &stationHwInfo[station-1], station ) ) break;
            }
        }
    }

    station = 1;
    lastTime = ISD_GetTime();

    while( !done )
    {
#if defined(_WIN32) || defined(WIN32) || defined(__WIN32__)
        if( _kbhit() )
        {
            switch( _getch() )
#else
	ch_key = unix_kbhit();
	if( ch_key != FALSE )
        {
            switch( ch_key )
#endif
            {
            case 'A':
                {
                    BYTE AuxOutput[4];

                    AuxOutput[0] = 255;
                    AuxOutput[1] = 255;
                    ISD_AuxOutput( handle, 1, AuxOutput, 4 );
                }
                break;

            case 'a':
                {
                    BYTE AuxOutput[4];

                    AuxOutput[0] = 0;
                    AuxOutput[1] = 0;
                    ISD_AuxOutput( handle, 1, AuxOutput, 4 );
                }
                break;

            case 'e': // IS-x products only, not for InterTrax 
            case 'E':
                
                // First get current station configuration 
                if( ISD_GetStationConfig( handle,
                    &Station[station-1], station, TRUE ) )
                {
                    Station[station-1].Enhancement = (Station[station-1].Enhancement+1) % 3;

                    // Send the new configuration to the tracker 
                    if( ISD_SetStationConfig( handle, 
                        &Station[station-1], station, TRUE ) )
                    {
                        // display the results 
                        showTrackerStats( handle, &hwInfo );
                    }
                }
                break;
   
	    case 't':
	    case 'T':
		 // First get current station configuration
                if( ISD_GetStationConfig( handle,
                    &Station[station-1], station, TRUE ) )
                {
                    Station[station-1].TimeStamped = !(Station[station-1].TimeStamped);

                    // Send the new configuration to the tracker
                    if( ISD_SetStationConfig( handle,
                        &Station[station-1], station, TRUE ) )
                    {
                        // display the results
                        showTrackerStats( handle, &hwInfo );
                    }
                }
	
		break;

	    case 'd':
	    case 'D':
		showTrackerStats( handle, &hwInfo );
		break;

	    case 'c':
            case 'C': // IS-x products only, not for InterTrax

                if( ISD_GetStationConfig( handle,
                    &Station[station-1], station, TRUE ))
                {
                    Station[station-1].Compass = (Station[station-1].Compass + 1) % 3;

                    if( ISD_SetStationConfig( handle,
                        &Station[station-1], station, TRUE ) )
                    {
                        showTrackerStats( handle, &hwInfo );
                    }
                }
                break;

            case 'p': 
            case 'P': // IS-x products only, not for InterTrax 
                
                if( ISD_GetStationConfig( handle, 
                    &Station[station-1], station, TRUE ))
                {
                    Station[station-1].Prediction = (Station[station-1].Prediction + 10) % 60;
                    
                    if( ISD_SetStationConfig( handle,
                        &Station[station-1], station, TRUE ) )
                    {
                        showTrackerStats( handle, &hwInfo );
                    }
                }
                break;
			case 's': 
            case 'S': // IS-x products only, not for InterTrax 
                
                if( ISD_GetStationConfig( handle, 
                    &Station[station-1], station, TRUE ))
                {
                    Station[station-1].Sensitivity = (Station[station-1].Sensitivity + 1) % 5;

					if( Station[station-1].Sensitivity == 0)
						Station[station-1].Sensitivity = 1;
                    
                    if( ISD_SetStationConfig( handle,
                        &Station[station-1], station, TRUE ) )
                    {
                        showTrackerStats( handle, &hwInfo );
                    }
                }
                break;

            case 'r':   
            case 'R':
                ISD_ResetHeading( handle, 1 );
                break;
                
            case '1':
                
                station = 1;
                printf("\n>> Current Station is set to %d <<\n", station);
                break;
                
            case '2': // IS-x products only, not for InterTrax 
                
                if( maxStations > 1 ) 
                {
                    station = 2;
                    printf("\n>> Current Station is set to %d <<\n", station);
                }
                break;
                
            case '3': // IS-x products only, not for InterTrax 
                
                if( maxStations > 2 ) 
                {
                    station = 3;
                    printf("\n>> Current Station is set to %d <<\n", station);
                }
                break;
                
            case '4': // IS-x products only, not for InterTrax 
                
                if( maxStations > 3 ) 
                {
                    station = 4;
                    printf("\n>> Current Station is set to %d <<\n", station);
                }
                break;

            case '5': // IS-x products only, not for InterTrax

                if( maxStations > 4 )
                {
                    station = 5;
                    printf("\n>> Current Station is set to %d <<\n", station);
                }
                break;

            case '6': // IS-x products only, not for InterTrax

                if( maxStations > 5 )
                {
                    station = 6;
                    printf("\n>> Current Station is set to %d <<\n", station);
                }
                break;

            case '7': // IS-x products only, not for InterTrax

                if( maxStations > 6 )
                {
                    station = 7;
                    printf("\n>> Current Station is set to %d <<\n", station);
                }
                break;

            case '8': // IS-x products only, not for InterTrax

                if( maxStations > 7 )
                {
                    station = 8;
                    printf("\n>> Current Station is set to %d <<\n", station);
                }
                break;

            case ESC:
            case 'q':
            case 'Q':
		printf( "\n\n" );
                done = TRUE;
                break;
                               
            default:
                printf( "\nq -- quit\n");
                printf( "1 -- make station 1 current\n" );
                printf( "2 -- make station 2 current\n" );
                printf( "3 -- make station 3 current\n" );
                printf( "4 -- make station 4 current\n" );
                printf( "5 -- make station 5 current\n" );
                printf( "6 -- make station 6 current\n" );
                printf( "7 -- make station 7 current\n" );
                printf( "8 -- make station 8 current\n" );
                printf( "D -- display current settings\n" );
                printf( "E -- cycle enhancement mode\n" );
                printf( "P -- cycle prediction\n" );
				printf( "C -- cycle compass\n" );
				printf( "S -- cycle sensitivity\n" );
                printf( "R -- reset heading\n" );
                break;
            }
        }
        
	// must be called at a reasonable rate
        if( handle > 0 )
        {
            ISD_GetTrackingData( handle, &data );    
            
            if( ISD_GetTime() - lastTime > 0.01f )
            {
                lastTime = ISD_GetTime();
                   
                if( handle > 0 )
                {
                    showStationData( handle, &Tracker,
                        &Station[station-1], &data.Station[station-1] );
                }
            }
        }
#ifdef _WIN32
        Sleep( 1 );
#endif
    }
    
    ISD_CloseTracker( handle );
    return 1;
}





