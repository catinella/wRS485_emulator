/*-------------------------------------------------------------------------------------------------------------------------------
//
//                                                 ____  ____  _  _    ___ ____  
//                                                |  _ \/ ___|| || |  ( _ ) ___| 
//                                                | |_) \___ \| || |_ / _ \___ \
//                                                |  _ < ___) |__   _| (_) |__) |
//                                                |_| \_\____/   |_|  \___/____/ 
//                                                           Emulator         
//
// 
// File:     RS485_emulator.h
// 
// Authour:  Silvano Catinella
// 
// Language: C
// 
// Description:
//	This file contains the RS485-emulator's custom codes. It will be included in the wError.h file.
//
//
-------------------------------------------------------------------------------------------------------------------------------*/


#define RS485EMULE_ERROR_GENERIC         0

//-------------------------------------------
//         S U C C E S S   C O D E S
//-------------------------------------------
#define RS485EMULE_SUCCESS               1
#define RS485EMULE_INFO_AVAILABLEPORT    3

//-------------------------------------------
//        W A R N I N G   C O D E S
//-------------------------------------------
#define RS485EMULE_WARNING_GENERIC       17
#define RS485EMULE_WARNING_ITEMNOTFOUND  19
#define RS485EMULE_WARNING_NOTHINGTODO   21
#define RS485EMULE_WARNING_TIMEOUT       23
#define RS485EMULE_WARNING_UNAVAILRES    25

//-------------------------------------------
//       B U S   E R R O R   C O D E S
//-------------------------------------------
#define RS485EMULE_ERROR_UNAVAILRES     129
#define RS485EMULE_ERROR_UNKNOWNARG     131
#define RS485EMULE_ERROR_IOFAILED       133
#define RS485EMULE_ERROR_INTERNAL       135
#define RS485EMULE_ERROR_FILENOTFOUND   137
#define RS485EMULE_ERROR_ILLEGALSYNTAX  139
#define RS485EMULE_ERROR_CORRUPTEDDATA  141
#define RS485EMULE_ERROR_DATAOVERFLOW   143
#define RS485EMULE_ERROR_NOSYSRESOURCE  145
#define RS485EMULE_ERROR_MISSINGDATA    147
#define RS485EMULE_ERROR_EXTPROCFAILED  149
#define RS485EMULE_ERROR_NOSPACE        151
#define RS485EMULE_ERROR_ITEMNOTFOUND   153
#define RS485EMULE_ERROR_FORBIDDENOP    155
#define RS485EMULE_ERROR_EXTTOOLFAILURE 157
#define RS485EMULE_ERROR_ILLEGALDATA    159
#define RS485EMULE_ERROR_ILLEGALARG     161
#define RS485EMULE_ERROR_UNKNOWN        163

//-------------------------------------------
// F A K E   D E V S   E R R O R   C O D E S
//                  [193-255]
//-------------------------------------------
	

