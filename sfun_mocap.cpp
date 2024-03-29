/*
 * sfuntmpl_basic.c: Basic 'C' template for a level 2 S-function.
 *
 * Copyright 1990-2018 The MathWorks, Inc.
 */


/*
 * You must specify the S_FUNCTION_NAME as the name of your S-function
 * (i.e. replace sfuntmpl_basic with the name of your S-function).
 */

#define S_FUNCTION_NAME  sfun_mocap
#define S_FUNCTION_LEVEL 2

/*
 * Need to include simstruc.h for the definition of the SimStruct and
 * its associated macro definitions.
 */
#include "simstruc.h"

// Include required headers for Qualisys RT Protocol.
#include "RTProtocol.h"
#include "RTPacket.h"

#ifdef _WIN32
#define sleep Sleep
#else
#include <unistd.h>
#endif

// Constants for RT Protocol
constexpr char           serverAddr[] = "192.168.244.1";
constexpr unsigned short basePort = 22222;
constexpr int            majorVersion = 1;
constexpr int            minorVersion = 22;
constexpr bool           bigEndian = false;
unsigned short udpPort = 6734;


/* Error handling
 * --------------
 *
 * You should use the following technique to report errors encountered within
 * an S-function:
 *
 *       ssSetErrorStatus(S,"Error encountered due to ...");
 *       return;
 *
 * Note that the 2nd argument to ssSetErrorStatus must be persistent memory.
 * It cannot be a local variable. For example the following will cause
 * unpredictable errors:
 *
 *      mdlOutputs()
 *      {
 *         char msg[256];         {ILLEGAL: to fix use "static char msg[256];"}
 *         sprintf(msg,"Error due to %s", string);
 *         ssSetErrorStatus(S,msg);
 *         return;
 *      }
 *
 */

/*====================*
 * S-function methods *
 *====================*/

/* Function: mdlInitializeSizes ===============================================
 * Abstract:
 *    The sizes information is used by Simulink to determine the S-function
 *    block's characteristics (number of inputs, outputs, states, etc.).
 */
static void mdlInitializeSizes(SimStruct *S)
{
    ssSetNumSFcnParams(S, 0);  /* Number of expected parameters */
    if (ssGetNumSFcnParams(S) != ssGetSFcnParamsCount(S)) {
        /* Return if number of expected != number of actual parameters */
        return;
    }
    
    ssSetNumContStates(S, 0);
    ssSetNumDiscStates(S, 0);
    
    if (!ssSetNumInputPorts(S, 0)) return;
    
    if (!ssSetNumOutputPorts(S, 1)) return;
    ssSetOutputPortWidth(S, 0, 6); // 6 DOFs
    
    ssSetNumSampleTimes(S, 1);
    ssSetNumRWork(S, 0);
    ssSetNumIWork(S, 0);
    // Reserve place for C++ object representing RT Protocol connection
    ssSetNumPWork(S, 1);
    ssSetNumModes(S, 0);
    ssSetNumNonsampledZCs(S, 0);
    
    /* Specify the operating point save/restore compliance to be same as a
     * built-in block */
    ssSetOperatingPointCompliance(S, USE_DEFAULT_OPERATING_POINT);
    
    ssSetRuntimeThreadSafetyCompliance(S, RUNTIME_THREAD_SAFETY_COMPLIANCE_TRUE);
    ssSetOptions(S, SS_OPTION_EXCEPTION_FREE_CODE);
}



/* Function: mdlInitializeSampleTimes =========================================
 * Abstract:
 *    This function is used to specify the sample time(s) for your
 *    S-function. You must register the same number of sample times as
 *    specified in ssSetNumSampleTimes.
 */
static void mdlInitializeSampleTimes(SimStruct *S)
{
    ssSetSampleTime(S, 0, CONTINUOUS_SAMPLE_TIME);
    ssSetOffsetTime(S, 0, 0.0);
    
}



#define MDL_INITIALIZE_CONDITIONS   /* Change to #undef to remove function */
#if defined(MDL_INITIALIZE_CONDITIONS)
/* Function: mdlInitializeConditions ========================================
 * Abstract:
 *    In this function, you should initialize the continuous and discrete
 *    states for your S-function block.  The initial states are placed
 *    in the state vector, ssGetContStates(S) or ssGetRealDiscStates(S).
 *    You can also perform any other initialization activities that your
 *    S-function may require. Note, this routine will be called at the
 *    start of simulation and if it is present in an enabled subsystem
 *    configured to reset states, it will be call when the enabled subsystem
 *    restarts execution to reset the states.
 */
static void mdlInitializeConditions(SimStruct *S)
{
}
#endif /* MDL_INITIALIZE_CONDITIONS */



#define MDL_START  /* Change to #undef to remove function */
#if defined(MDL_START)
/* Function: mdlStart =======================================================
 * Abstract:
 *    This function is called once at start of model execution. If you
 *    have states that should be initialized once, this is the place
 *    to do it.
 */
static void mdlStart(SimStruct *S)
{
    // Attempt to connect with Qualisys using RT Protocol
    try
    {
        CRTProtocol* rtProtocol = new CRTProtocol();
        
        bool dataAvailable = false;
        bool streamFrames = false;
        
        // Connect with Qualisys system
        ssPrintf("Checking connection...\n");
        if (!rtProtocol->Connected())
        {
            ssPrintf("Attempting to connect...\n");
            if (!rtProtocol->Connect(serverAddr, basePort, &udpPort, majorVersion, minorVersion, bigEndian))
            {
                ssPrintf("rtProtocol.Connect: %s\n\n", rtProtocol->GetErrorString());
                // TODO: handle failed connection
            }
        }
        
        if (!dataAvailable)
        {
            if (!rtProtocol->Read6DOFSettings(dataAvailable))
            {
                ssPrintf("rtProtocol.Read6DOFSettings: %s\n\n", rtProtocol->GetErrorString());
                // TODO: handle failed connection
                
            }
        }
        
        if (!streamFrames)
        {
            if (!rtProtocol->StreamFrames(CRTProtocol::RateAllFrames, 0, udpPort, NULL, CRTProtocol::cComponent6d))
            {
                ssPrintf("rtProtocol.StreamFrames: %s\n\n", rtProtocol->GetErrorString());
                // TODO: handle failed connection
                
            }
            streamFrames = true;
            
            ssPrintf("Starting to stream 6DOF data\n\n");
        }
        
        // Store RT Protocol object in pointers vector
        ssGetPWork(S)[0] = rtProtocol;
    }
    catch (std::exception& e)
    {
        ssPrintf("%s\n", e.what());
    }
}
#endif /*  MDL_START */



/* Function: mdlOutputs =======================================================
 * Abstract:
 *    In this function, you compute the outputs of your S-function
 *    block.
 */
static void mdlOutputs(SimStruct *S, int_T tid)
{
    // Get Output port
    real_T* posOutput = (real_T*)ssGetOutputPortSignal(S,0);
    
    // Retrieve RT Protocol object from pointers vector
    CRTProtocol* rtProtocol = static_cast<CRTProtocol*>(ssGetPWork(S)[0]);
    
    CRTPacket::EPacketType packetType;
    
    if (rtProtocol->Receive(packetType, true) == CNetwork::ResponseType::success)
    {
        if (packetType == CRTPacket::PacketData)
        {
            float fX, fY, fZ;
            float fAng1, fAng2, fAng3;
            
            CRTPacket* rtPacket = rtProtocol->GetRTPacket();
            
            ssPrintf("Frame %d\n", rtPacket->GetFrameNumber());
            ssPrintf("======================================================================================================================\n");
            
            for (unsigned int i = 0; i < rtPacket->Get6DOFEulerBodyCount(); i++)
            {
                if (rtPacket->Get6DOFEulerBody(i, fX, fY, fZ, fAng1, fAng2, fAng3))
                {
                    const char* pTmpStr = rtProtocol->Get6DOFBodyName(i);
                    if (pTmpStr)
                    {
                        ssPrintf("%-12s ", pTmpStr);
                    }
                    else
                    {
                        ssPrintf("Unknown     ");
                    }
                    ssPrintf("Pos: %9.3f %9.3f %9.3f    Roll: %3.3f    Pitch: %3.3f    Yaw: %3.3f\n",
                            fX, fY, fZ, fAng1, fAng2, fAng3);
                }
            }
            ssPrintf("\n");
            
            const float position[6]{fX, fY, fZ, fAng1, fAng2, fAng3};
            
            // Set outputs (NOTE: if >1 body, will only reflect coordinates of the last one)
            
            for (int i = 0; i < ssGetOutputPortWidth(S, 0); i++)
                posOutput[i] = position[i];
        }
    }
    rtProtocol->StopCapture();
}



#undef MDL_UPDATE  /* Change to #undef to remove function */
#if defined(MDL_UPDATE)
/* Function: mdlUpdate ======================================================
 * Abstract:
 *    This function is called once for every major integration time step.
 *    Discrete states are typically updated here, but this function is useful
 *    for performing any tasks that should only take place once per
 *    integration step.
 */
static void mdlUpdate(SimStruct *S, int_T tid)
{
}
#endif /* MDL_UPDATE */



#undef MDL_DERIVATIVES  /* Change to #undef to remove function */
#if defined(MDL_DERIVATIVES)
/* Function: mdlDerivatives =================================================
 * Abstract:
 *    In this function, you compute the S-function block's derivatives.
 *    The derivatives are placed in the derivative vector, ssGetdX(S).
 */
static void mdlDerivatives(SimStruct *S)
{
}
#endif /* MDL_DERIVATIVES */



/* Function: mdlTerminate =====================================================
 * Abstract:
 *    In this function, you should perform any actions that are necessary
 *    at the termination of a simulation.  For example, if memory was
 *    allocated in mdlStart, this is the place to free it.
 */
static void mdlTerminate(SimStruct *S)
{
    // Retrieve RT Protocol object from pointers vector
    CRTProtocol* rtProtocol = static_cast<CRTProtocol*>(ssGetPWork(S)[0]);
    
    // Disconnect from Qualisys system
    rtProtocol->Disconnect();
    
    // Delete object
    delete rtProtocol;
}


/*=============================*
 * Required S-function trailer *
 *=============================*/

#ifdef  MATLAB_MEX_FILE    /* Is this file being compiled as a MEX-file? */
#include "simulink.c"      /* MEX-file interface mechanism */
#else
#include "cg_sfun.h"       /* Code generation registration function */
#endif

