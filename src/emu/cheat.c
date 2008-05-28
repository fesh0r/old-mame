/*********************************************************************

    cheat.c

    MAME cheat system.

    Copyright Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

*********************************************************************/

#include "driver.h"
#include "ui.h"
#include "uimenu.h"
#include "machine/eeprom.h"
#include "cheat.h"
#include <ctype.h>
#include <math.h>

#ifdef MESS
#include "cheatms.h"
#endif

#define OSD_READKEY_KLUDGE	1

/***************************************************************************
    MACROS
***************************************************************************/

/* easy bitfield extraction and setting, uses *_Shift, *_ShiftedMask, and *_Mask enums */
#define EXTRACT_FIELD(data, name)				(((data) >> k##name##_Shift) & k##name##_ShiftedMask)
#define SET_FIELD(data, name, in)				(data = (data & ~(k##name##_ShiftedMask << k##name##_Shift)) | (((in) & k##name##_ShiftedMask) << k##name##_Shift))
#define TEST_FIELD(data, name)					((data) & k##name##_Mask)
#define SET_MASK_FIELD(data, name)				((data) |= k##name##_Mask)
#define CLEAR_MASK_FIELD(data, name)			((data) &= ~(k##name##_Mask))
#define TOGGLE_MASK_FIELD(data, name)			((data) ^= k##name##_Mask)

#define DEFINE_BITFIELD_ENUM(name, end, start)	k##name##_Shift = (int)(end), 											\
												k##name##_ShiftedMask = (int)(0xFFFFFFFF >> (32 - (start - end + 1))),	\
												k##name##_Mask = (int)(k##name##_ShiftedMask << k##name##_Shift)

#define REGION_LIST_LENGTH						(REGION_MAX - REGION_INVALID)
#define VALID_CPU(cpu)							(cpu < cpu_gettotalcpu())

#define ADD_MENU_2_ITEMS(name, sub_name)		do { menu_item[total] = name; menu_sub_item[total] = sub_name; total++; } while(0)
#define TERMINATE_MENU_2_ITEMS					do { menu_item[total] = NULL; menu_sub_item[total] = NULL; } while(0)

#define ADD_MENU_3_ITEMS(name, sub_name, enum)	do { menu_item[total] = name; menu_sub_item[total] = sub_name; menu_index[total] = enum; total++; } while(0)

#define	ADJUST_CURSOR(sel, total)				if(sel < 0) sel = 0; else if(sel > (total - 1)) sel = (total - 1);
#define	CURSOR_TO_NEXT(sel, total)				if(++sel > (total - 1)) sel = 0;
#define	CURSOR_TO_PREVIOUS(sel, total)			if(--sel < 0) sel = (total - 1);
#define	CURSOR_PAGE_UP(sel)						sel -= visible_items; if(sel < 0) sel = 0;
#define CURSOR_PAGE_DOWN(sel, total)			sel += visible_items; if(sel >= total) sel = (total - 1);

/***************************************************************************
    CONSTANTS
***************************************************************************/

#define CHEAT_FILENAME_MAX_LEN					(255)
#define CHEAT_MENU_DEPTH						(8)
#ifdef MESS
#define DEFAULT_CHEAT_OPTIONS					(0x4F)
#else
#define DEFAULT_CHEAT_OPTIONS					(0xF)
#endif
#define DEFAULT_MESSAGE_TIME					(50)
#define VARIABLE_MAX_ARRAY						(0x0F)		// NOTE : Move code can accept max 16 variables as destination
#define CHEAT_RETURN_VALUE						(0xFF)
#define CUSTOM_CODE								(0xFF)

/********** BIT FIELD **********/

enum
{
	/* type field */
	DEFINE_BITFIELD_ENUM(OneShot,					0,	0),
	DEFINE_BITFIELD_ENUM(DelayEnable,				1,	1),
	DEFINE_BITFIELD_ENUM(PrefillEnable,				2,	2),
	DEFINE_BITFIELD_ENUM(Return,					3,	3),
	DEFINE_BITFIELD_ENUM(RestoreValue,				4,	4),
	DEFINE_BITFIELD_ENUM(DataRead,					5,	5),		// 0 = field, 1 = variable
	DEFINE_BITFIELD_ENUM(AddressRead,				6,	7),		// 0 = field, 1 = indirect address from variable, 2 = variable
	DEFINE_BITFIELD_ENUM(CodeParameter,				8,	11),
	DEFINE_BITFIELD_ENUM(CodeType,					12,	15),
	DEFINE_BITFIELD_ENUM(Link,						16,	17),	// 0 = master, 1 = link, 2 = sub link for label-selection, 3 = copy link for VWrite, VRWrite
	DEFINE_BITFIELD_ENUM(LinkValueSelectBCD,		18,	18),	// used by VWrite (link)
	DEFINE_BITFIELD_ENUM(LinkValueSelectNegative,	19,	19),	// used by VWrite (link)
	DEFINE_BITFIELD_ENUM(PopupParameter,			18,	19),	// used by Popup
	DEFINE_BITFIELD_ENUM(AddressSize,				20,	21),
	DEFINE_BITFIELD_ENUM(IndexSize,					22,	23),	// used by IWrite, VWrite, Move, Popup
	DEFINE_BITFIELD_ENUM(CPURegion,					24, 31),	// extract to CPU/Region field

	/* extend field */
	DEFINE_BITFIELD_ENUM(LSB16,						0,	15),	// lower 4 bytes
	DEFINE_BITFIELD_ENUM(MSB16,						16,	31),	// upper 4 bytes

	/* value-select master code */
	DEFINE_BITFIELD_ENUM(ValueSelectMinimumDisplay,	8,	8),
	DEFINE_BITFIELD_ENUM(ValueSelectMinimum,		9,	9),
	DEFINE_BITFIELD_ENUM(ValueSelectBCD,			10,	10),
	DEFINE_BITFIELD_ENUM(ValueSelectNegative,		11,	11),

	/* watch code */
	DEFINE_BITFIELD_ENUM(WatchDisplayFormat,		8,	9),		// from type field
	DEFINE_BITFIELD_ENUM(WatchShowLabel,			10,	10),	//
	DEFINE_BITFIELD_ENUM(WatchNumElements,			0,	7),		// from data field
	DEFINE_BITFIELD_ENUM(WatchSkip,					8,	15),	//
	DEFINE_BITFIELD_ENUM(WatchElementsPerLine,		16,	23),	//
	DEFINE_BITFIELD_ENUM(WatchAddValue,				24,	31),	//

	/* label-selection */
	DEFINE_BITFIELD_ENUM(LabelSelectUseSelector,	4,	4),
	DEFINE_BITFIELD_ENUM(LabelSelectQuickClose,		5,	5),

	/* cpu/region field */
	DEFINE_BITFIELD_ENUM(CPUIndex,					0,	3),		// cpu region
	DEFINE_BITFIELD_ENUM(AddressSpace,				4,	6),		// cpu region
	DEFINE_BITFIELD_ENUM(RegionIndex,				0,	6),		// non-cpu region
	DEFINE_BITFIELD_ENUM(RegionFlag,				7,	7),		// 0 = cpu, 1 = non-cpu

	/* old code */
//  DEFINE_BITFIELD_ENUM(OneShot,                   0,  0),
	DEFINE_BITFIELD_ENUM(Type,						1,	2),
	DEFINE_BITFIELD_ENUM(Operation,					3,	4),
	DEFINE_BITFIELD_ENUM(TypeParameter,				5,	7),
	DEFINE_BITFIELD_ENUM(UserSelectEnable,			8,	8),
	DEFINE_BITFIELD_ENUM(UserSelectMinimumDisplay,	9,	9),
	DEFINE_BITFIELD_ENUM(UserSelectMinimum,			10,	10),
	DEFINE_BITFIELD_ENUM(UserSelectBCD,				11,	11),
	DEFINE_BITFIELD_ENUM(Prefill,					12,	13),
	DEFINE_BITFIELD_ENUM(RemoveFromList,			14, 14),
	DEFINE_BITFIELD_ENUM(LinkExtension,				15,	15),
	DEFINE_BITFIELD_ENUM(LinkEnable,				16,	16),
	DEFINE_BITFIELD_ENUM(LinkCopyPreviousValue,		17,	17),
	DEFINE_BITFIELD_ENUM(OperationParameter,		18,	18),
	DEFINE_BITFIELD_ENUM(OperationExtend,			19,	19),
	DEFINE_BITFIELD_ENUM(BytesUsed,					20,	21),
	DEFINE_BITFIELD_ENUM(Endianness,				22,	22),
	DEFINE_BITFIELD_ENUM(RestorePreviousValue,		23, 23),
	DEFINE_BITFIELD_ENUM(LocationParameter,			24,	28),
	DEFINE_BITFIELD_ENUM(LocationType,				29,	31),

	/* location parameter extention (relative address, memory accessor) */
	DEFINE_BITFIELD_ENUM(LocationParameterOption,	24,	25),
	DEFINE_BITFIELD_ENUM(LocationParameterCPU,		26,	28),

	/* command */
	DEFINE_BITFIELD_ENUM(SearchBox,					0,	1),
	DEFINE_BITFIELD_ENUM(DontPrintNewLabels,		2,	2),		// in options menu, it is reversed display
	DEFINE_BITFIELD_ENUM(AutoSaveEnabled,			3,	3),
	DEFINE_BITFIELD_ENUM(ActivationKeyMessage,		4,	4),
	DEFINE_BITFIELD_ENUM(LoadOldFormat,				5,	5),
#ifdef MESS
	DEFINE_BITFIELD_ENUM(SharedCode,				6,	6)
#endif
};

/********** OPERATION **********/

enum{		// address space for RAM region
	kAddressSpace_Program = 0,
	kAddressSpace_DataSpace,
	kAddressSpace_IOSpace,
	kAddressSpace_OpcodeRAM,
	kAddressSpace_MappedMemory,
	kAddressSpace_DirectMemory,
	kAddressSpace_EEPROM };

enum{		// code types
	kCodeType_Write = 0,		// EF = Mask
	kCodeType_IWrite,			// EF = Index
	kCodeType_RWrite,			// EF = Count/Skip
	kCodeType_CWrite,			// EF = Condition
	kCodeType_VWrite,			// EF = Index
	kCodeType_VRWrite,			// EF = Count/Skip
	kCodeType_PDWWrite,			// EF = 2nd Data
	kCodeType_Move,				// EF = Index
	kCodeType_Branch,			// EF = Jump Code
	kCodeType_Loop,				// EF = Index
	kCodeType_Unused1,
	kCodeType_Unused2,
	kCodeType_Unused3,
	kCodeType_Unused4,
	kCodeType_Popup,			// EF = Index
	kCodeType_Watch };			// EF = X and Y Position

enum{		// parameter for IWrite
	kIWrite_Write = 0,
	kIWrite_BitSet,
	kIWrite_BitClear,
	kIWrite_LimitedMask };

enum{		// conditions for CWrite, Branch, Popup
	kCondition_Equal = 0,
	kCondition_NotEqual,
	kCondition_Less,
	kCondition_LessOrEqual,
	kCondition_Greater,
	kCondition_GreaterOrEqual,
	kCondition_BitSet,
	kCondition_BitClear,
	kCondition_Unused1,
	kCondition_Unused2,
	kCondition_Unused3,
	kCondition_Unused4,
	kCondition_PreviousValue,
	kCondition_KeyPressedOnce,
	kCondition_KeyPressedRepeat,
	kCondition_True };

enum{		// popup parameter
	kPopup_Label = 0,
	kPopup_Value,
	kPopup_LabelValue,
	kPopup_ValueLabel };

enum{		// parameter for custom
	kCustomCode_NoAction = 0,
	kCustomCode_Comment,
	kCustomCode_Separator,
	kCustomCode_LabelSelect,
	kCustomCode_Layer,
	kCustomCode_Unused1,
	kCustomCode_Unused2,
	kCustomCode_Unused3,
	kCustomCode_ActivationKey1,
	kCustomCode_ActivationKey2,
	kCustomCode_PreEnable,
	kCustomCode_OverClock,
	kCustomCode_RefreshRate };

enum{		// link level
	kLink_Master = 0,
	kLink_Link,
	kLink_SubLink,
	kLink_CopyLink };

enum{		// address read from
	kReadFrom_Address = 0,			// $xxxx
	kReadFrom_Index,				// $Vx
	kReadFrom_Variable };			// Vx

enum		// types
{
	kType_NormalOrDelay = 0,
	kType_WaitForModification,
	kType_IgnoreIfDecrementing,
	kType_Watch
};

enum		// operations
{
	kOperation_WriteMask = 0,
	kOperation_AddSubtract,
	kOperation_ForceRange,
	kOperation_SetOrClearBits,
	kOperation_Unused4,
	kOperation_Unused5,
	kOperation_Unused6,
	kOperation_None
};

//enum      // prefills (unused?)
//{
//  kPrefill_Disable = 0,
//  kPrefill_UseFF,
//  kPrefill_Use00,
//  kPrefill_Use01
//};

enum		// location types
{
	kLocation_Standard = 0,
	kLocation_MemoryRegion,
	kLocation_HandlerMemory,
	kLocation_Custom,
	kLocation_IndirectIndexed,
	kLocation_Unused5,
	kLocation_Unused6,
	kLocation_Unused7
};

enum		// location parameters
{
	kCustomLocation_Comment = 0,
	kCustomLocation_EEPROM,
	kCustomLocation_Select,
	kCustomLocation_AssignActivationKey,
	kCustomLocation_Enable,
	kCustomLocation_Overclock,
	kCustomLocation_RefreshRate
};

enum		// memory accessor parameters
{
	kMemoryLocation_DirectAccess = 0,
	kMemoryLocation_DataSpace,
	kMemoryLocation_IOSpace,
	kMemoryLocation_OpbaseRAM
};

enum		// action flags
{
	/* set if this code is not the newest format */
	kActionFlag_OldFormat =			1 << 0,

	/* set if this code doesn't need to read/write a value */
	kActionFlag_NoAction =			1 << 1,

	/* set if this code is custom cheat */
	kActionFlag_Custom =			1 << 2,

	/* set for one shot cheats after the operation is performed */
	kActionFlag_OperationDone =		1 << 3,

	/* set if the extendData field is being used by index adress reading (IWrite, VWrite, Move, Loop, Popup)*/
	kActionFlag_IndexAddress =		1 << 4,

	/* set if the code needs to check the condition (CWrite, Branch, Popup) */
	kActionFlag_CheckCondition =	1 << 5,

	/* set if the code needs to repeat writing (RWrite, VRWrite) */
	kActionFlag_Repeat =			1 << 6,

	/* set if the extendData field is being used by 2nd data (PDWWrite) */
	kActionFlag_PDWWrite =			1 << 7,

	/* set if the lastValue field contains valid data and can be restored if needed */
	kActionFlag_LastValueGood =		1 << 8,

	/* set after value changes from prefill value */
	kActionFlag_PrefillDone =		1 << 9,

	/* set after prefill value written */
	kActionFlag_PrefillWritten =	1 << 10,

	/* set if the code is a label for label-selection code */
	kActionFlag_IsLabel =			1 << 11,

	/* set when read/write a data after 1st read/write and clear after 2nd read/write (PDWWrite) */
	kActionFlag_IsFirst =			1 << 12,

	/* masks */
	kActionFlag_StateMask =			kActionFlag_OperationDone |		// used in ResetAction() to clear specified flags
									kActionFlag_LastValueGood |
									kActionFlag_PrefillDone |
									kActionFlag_PrefillWritten,
	kActionFlag_InfoMask =			kActionFlag_IndexAddress,		// ??? unused ???
	kActionFlag_PersistentMask =	kActionFlag_OldFormat |			// used in UpdateCheatInfo() to clear other flags
									kActionFlag_LastValueGood
};

enum		// entry flags
{
	/* true when the cheat is active */
	kCheatFlag_Active =					1 << 0,

	/* true if the cheat is entirely one shot */
	kCheatFlag_OneShot =				1 << 1,

	/* true if the cheat is entirely null (ex. a comment) */
	kCheatFlag_Null =					1 << 2,

	/* true if the cheat is multi-comment code */
	kCheatFlag_ExtendComment =			1 << 3,

	/* true if the cheat is specified separator */
	kCheatFlag_Separator =				1 << 4,

	/* true if the cheat contains a user-select element */
	kCheatFlag_UserSelect =				1 << 5,

	/* true if the cheat is a select cheat */
	kCheatFlag_Select =					1 << 6,

	/* true if the cheat is layer index cheat */
	kCheatFlag_LayerIndex =				1 << 7,

	/* true if selected code is layer index cheat */
	kCheatFlag_LayerSelected =			1 << 8,

	/* true if the cheat has been assigned an 1st activation key */
	kCheatFlag_HasActivationKey1 =		1 << 9,

	/* true if the cheat has been assigned an 2nd activation key */
	kCheatFlag_HasActivationKey2 =		1 << 10,

	/* true if the cheat is not the newest format */
	kCheatFlag_OldFormat =				1 << 11,

	/* true if wrong cheat code */
	kCheatFlag_HasWrongCode =			1 << 12,

	/* true if the cheat has been edited or is a new cheat
       checked at auto-save then save the code if true */
	kCheatFlag_Dirty =					1 << 13,

	/* masks */
	kCheatFlag_StateMask =			kCheatFlag_Active,					// used in ResetAction() and DeactivateCheat() to clear specified flag
	kCheatFlag_InfoMask =			kCheatFlag_OneShot |				// used in UpdateCheatInfo() to detect specified flags
									kCheatFlag_Null |
									kCheatFlag_ExtendComment |
									kCheatFlag_Separator |
									kCheatFlag_UserSelect |
									kCheatFlag_Select |
									kCheatFlag_LayerIndex |
									kCheatFlag_OldFormat,
	kCheatFlag_PersistentMask =		kCheatFlag_Active |					// used in UpdateCheatInfo() to extract specified flags
									kCheatFlag_HasActivationKey1 |
									kCheatFlag_HasActivationKey2 |
									kCheatFlag_Dirty
};

/********** WATCH **********/

enum		// types
{
	kWatchLabel_None = 0,
	kWatchLabel_Address,
	kWatchLabel_String,

	kWatchLabel_MaxPlusOne
};

enum		// display types
{
	kWatchDisplayType_Hex = 0,
	kWatchDisplayType_Decimal,
	kWatchDisplayType_Binary,
	kWatchDisplayType_ASCII,

	kWatchDisplayType_MaxPlusOne
};

/********** REGION **********/

enum		// flags
{
	/* true if enabled for search */
	kRegionFlag_Enabled =		1 << 0

	/* true if the memory region has no mapped memory and uses a memory handler (unused?) */
//  kRegionFlag_UsesHandler =   1 << 1
};

enum		// types
{
	kRegionType_CPU = 0,
	kRegionType_Memory
};

enum		// default search regions
{
	ksearch_speed_Fast = 0,		// RAM + some banks
	ksearch_speed_Medium,		// RAM + BANKx
	ksearch_speed_Slow,			// all memory areas except ROM, NOP, and custom handlers
	ksearch_speed_VerySlow,		// all memory areas except ROM and NOP
	ksearch_speed_AllMemory,		// entire CPU address space
	ksearch_speed_UserDefined,	// user defined region

	ksearch_speed_Max
};

/********** SEARCH **********/

enum		// operands
{
	kSearchOperand_Current = 0,
	kSearchOperand_Previous,
	kSearchOperand_First,
	kSearchOperand_Value,

	kSearchOperand_Max = kSearchOperand_Value
};

enum		// length
{
	kSearchSize_8Bit = 0,
	kSearchSize_16Bit,
	kSearchSize_24Bit,
	kSearchSize_32Bit,
	kSearchSize_1Bit,

	kSearchSize_Max = kSearchSize_1Bit
};

enum		// comparisons
{
	kSearchComparison_LessThan = 0,
	kSearchComparison_GreaterThan,
	kSearchComparison_EqualTo,
	kSearchComparison_LessThanOrEqualTo,
	kSearchComparison_GreaterThanOrEqualTo,
	kSearchComparison_NotEqual,
	kSearchComparison_IncreasedBy,
	kSearchComparison_NearTo,

	kSearchComparison_Max = kSearchComparison_NearTo
};

/********** OTHERS **********/

enum
{
	kVerticalKeyRepeatRate =		8,
	kHorizontalFastKeyRepeatRate =	5,
	kHorizontalSlowKeyRepeatRate =	8
};

enum		// search menu
{
	kSearchBox_Minimum = 0,
	kSearchBox_Standard,
	kSearchBox_Advanced
};

enum		// load flags
{
	kLoadFlag_CheatOption	= 1,
	kLoadFlag_CheatCode		= 1 << 1,
	kLoadFlag_UserRegion	= 1 << 2
};

enum		// message types
{
	kCheatMessage_None = 0,
	kCheatMessage_ReloadCheatOption,
	kCheatMessage_ReloadCheatCode,
	kCheatMessage_ReloadUserRegion,
	kCheatMessage_FailedToLoadDatabase,
	kCheatMessage_CheatFound,
	kCheatMessage_1CheatFound,
	kCheatMessage_SucceededToSave,
	kCheatMessage_FailedToSave,
	kCheatMessage_AllCheatSaved,
	kCheatMessage_ActivationKeySaved,
	kCheatMessage_NoActivationKey,
	kCheatMessage_PreEnableSaved,
	kCheatMessage_SucceededToAdd,
	kCheatMessage_FailedToAdd,
	kCheatMessage_SucceededToDelete,
	kCheatMessage_FailedToDelete,
	kCheatMessage_NoSearchRegion,
	kCheatMessage_RestoreValue,
	kCheatMessage_NoOldValue,
	kCheatMessage_InitializeMemory,
	kCheatMessage_InvalidatedRegion,
	kCheatMessage_DebugOnly,
	kCheatMessage_WrongCode,

	kCheatMessage_Max
};

enum{		// error flags
	kErrorFlag_InvalidLocationType		= 1,		// old
	kErrorFlag_InvalidOperation			= 1 << 1,	// old
	kErrorFlag_InvalidCodeType			= 1 << 2,	// new
	kErrorFlag_InvalidCondition			= 1 << 3,	// new
	kErrorFlag_InvalidCodeOption		= 1 << 4,	// new
	kErrorFlag_ConflictedExtendField	= 1 << 5,	// old
	kErrorFlag_InvalidRange				= 1 << 6,
	kErrorFlag_NoRestorePreviousValue	= 1 << 7,
	kErrorFlag_NoLabel					= 1 << 8,
	kErrorFlag_ConflictedSelects		= 1 << 9,
	kErrorFlag_CopyLinkWithoutSelect	= 1 << 10,	// new
	kErrorFlag_InvalidDataField			= 1 << 11,
	kErrorFlag_InvalidExtendDataField	= 1 << 12,
	kErrorFlag_ForbittenVariable		= 1 << 13,	// new
	kErrorFlag_NoSelectableValue		= 1 << 14,
	kErrorFlag_NoRepeatCount			= 1 << 15,	// new
	kErrorFlag_UndefinedAddressRead		= 1 << 16,	// new
	kErrorFlag_AddressVariableOutRange	= 1 << 17,	// new
	kErrorFlag_DataVariableOutRange		= 1 << 18,	// new
	kErrorFlag_OutOfCPURegion			= 1 << 19,
	kErrorFlag_InvalidCPURegion			= 1 << 20,
	kErrorFlag_InvalidAddressSpace		= 1 << 21,	// new
	kErrorFlag_RegionOutOfRange			= 1 << 22,
	kErrorFlag_InvalidCustomCode		= 1 << 23,	// new

	kErrorFlag_Max						= 24 };

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

/********** ACTION **********/

struct CheatAction
{
	UINT32	type;					// packed several information (cpu/region, address size, operation and parameters and so on)
	UINT8	region;					// the number of cpu or region. in case of cpu, it has a kind of an address space together
	UINT32	address;				// address. it is changeable by RWrite in the cheat core
	UINT32	originalAddress;		// address. it is for edit/view/save
	UINT32	data;					// data. it is changeable by VWrite/VRWrite in the cheat core
	UINT32	originalDataField;		// data. it is for edit/view/save
	UINT32	extendData;				// extend parameter. it is different using by each operations

	INT32	frameTimer;				// timer for delay, keep
	UINT32	* lastValue;			// back up value before cheating to restore value. PDWWrite uses 2 and RWrite uses about a counter

	UINT32	flags;					// internal flags

	UINT8	** cachedPointer;		// pointer for data, I/O space or non-cpu region
	UINT32	cachedOffset;			// offset for mapped memory

	char	* optionalName;			// name to display in edit/view menu. Action doesn't have a comment
};

typedef struct CheatAction	CheatAction;

/********** ENTRY **********/

struct CheatEntry
{
	char			* name;					// name to display in list menu
	char			* comment;				// comment

	INT32			actionListLength;		// minimum length = 1
	CheatAction		* actionList;

	int				activationKey1;			// activation key index for 1st key
	int				activationKey2;			// activation key index for 2nd key

	int				selection;				// for label-select to set selected label number
	int				labelIndexLength;		// minimum length = 1
	int				* labelIndex;			// label index table
	int				layerIndex;				// layer index for enable/disable menu

	UINT32			flags;					// internal flags
};

typedef struct CheatEntry	CheatEntry;

/********** WATCH **********/

struct WatchInfo
{
	UINT32			address;
	UINT8			cpu;
	UINT8			numElements;
	UINT8			elementBytes;
	UINT8			labelType;
	UINT8			displayType;
	UINT8			skip;
	UINT8			elementsPerLine;
	INT8			addValue;
	INT8			addressShift;
	INT8			dataShift;
	UINT32			xor;

	float			x, y;

	CheatEntry *	linkedCheat;

	char			label[256];
};

typedef struct WatchInfo	WatchInfo;

/********** REGION **********/

typedef struct _search_region search_region;
struct _search_region
{
	UINT32	address;
	UINT32	length;

	UINT8	target_type;
	UINT8	target_idx;

	UINT8	flags;

	UINT8	** cached_pointer;
	const address_map_entry
			* write_handler;

	UINT8	* first;
	UINT8	* last;

	UINT8	* status;

	UINT8	* backup_last;
	UINT8	* backup_status;

	char	name[64];

	UINT32	num_results;
	UINT32	old_num_results;
};

/********** SEARCH **********/

typedef struct _old_search_options old_search_options;
struct _old_search_options
{
	INT8	comparison;
	UINT32	value;
	INT8	sign;
	UINT32	delta;
};

typedef struct _search_info search_info;
struct _search_info
{
	INT32				region_list_length;
	search_region		*region_list;

	char				*name;			/* search info name used in select_region() */

	INT8				bytes;			/* 0 = 1, 1 = 2, 2 = 3, 3 = 4, 4 = bit */
	UINT8				swap;
	UINT8				sign;
	INT8				lhs;
	INT8				rhs;
	INT8				comparison;

	UINT8				target_type;		/* is cpu or region? */
	UINT8				target_idx;			/* index for cpu or region */

	UINT32				value;

	INT8				search_speed;

	UINT32				num_results;
	UINT32				old_num_results;

	INT32				current_region_idx;
	INT32				current_results_page;

	UINT8				backup_valid;

	old_search_options	old_options;
};

/********** CPU **********/

typedef struct _cpu_region_info cpu_region_info;
struct _cpu_region_info
{
	UINT8	type;					/* cpu or region type */
	UINT8	data_bits;				/* data bits */
	UINT8	address_bits;			/* address bits */
	UINT8	address_chars_needed;	/* length of address */
	UINT32	address_mask;			/* address mask */
	UINT8	endianness;				/* endianness */
	UINT8	address_shift;			/* address shift */
};

/********** STRINGS **********/

typedef struct _menu_string_list menu_string_list;
struct _menu_string_list
{
	const char	**main_list;	/* main item field in menu items */
	const char	**sub_list;		/* sub item field in menu items */
	char		*flag_list;		/* flag of highlight for sub menu field */

	char	**main_strings;		/* strings buffer 1 */
	char	**sub_strings;		/* strings buffer 2 */

	char	*buf;				/* internal buffer? (only used in rebuild_string_tables()) */

	UINT32	length;				/* number of total menu items */
	UINT32	num_strings;		/* number of strings buffer */
	UINT32	main_string_length;	/* length of string in strings buffer 1 */
	UINT32	sub_string_length;	/* length of string in strings buffer 2 */
};

/********** MENUS **********/

/* NOTE : _cheat_menu_item_info is used in the edit menu right now */
typedef struct _cheat_menu_item_info cheat_menu_item_info;
struct _cheat_menu_item_info
{
	UINT32	sub_cheat;		/* the number of an item */
	UINT32	field_type;		/* field type of an item */
	UINT32	extra_data;		/* it stores an extra data but NOT read (so I don't know how to use)*/
};

typedef struct _cheat_menu_stack cheat_menu_stack;
typedef int (*cheat_menu_handler)(running_machine *machine, cheat_menu_stack *menu);
struct _cheat_menu_stack
{
	cheat_menu_handler	handler;		/* menu handler for cheat */
	cheat_menu_handler	return_handler;	/* menu handler to return menu */
	int					sel;			/* current cursor position */
	int					pre_sel;		/* selected item in previous menu */
	UINT8				first_time;		/* flags to first setting (1 = first, 0 = not first) */
};

/********** FORMAT **********/
typedef const struct _cheat_format cheat_format;
struct _cheat_format
{
	const char *	format_string;			/* string templete of a format */
	UINT8			arguments_matched;		/* valid arguments of a format */
	UINT8			length_matched;			/* valid length of a parameter */
	UINT8			comment_matched;		/* valid argument to add comment */
};

static const struct _cheat_format cheat_format_table[] =
{
		/* option - :_command:TYPE */
	{	":_command:%[^:\n\r]",												1,	8,	0	},
#ifdef MESS
		/* code (new) - >MACHINE_NAME:CRC:TYPE:ADDRESS:DATA:EXTEND_DATA:(DESCRIPTION:COMMENT) */
	{	">%8[^:\n\r]:%8X:%8X:%X:%8[^:\n\r]:%8[^:\n\r]:%[^:\n\r]:%[^:\n\r]",	6,	8,	8	},
		/* code (old) - :MACHINE_NAME:CRC:TYPE:ADDRESS:DATA:EXTEND_DATA:(DESCRIPTION:COMMENT) */
	{	":%8s:%8X:%8X:%X:%8[^:\n\r]:%8[^:\n\r]:%[^:\n\r]:%[^:\n\r]",		6,	8,	0	},
		/* code (older) - :MACHINE_NAME:CRC:CPU:ADDRESS:DATA:TYPE:(DESCRIPTION:COMMENT) */
	{	"%8[^:\n\r]:%8X:%d:%X:%X:%d:%[^:\n\r]:%[^:\n\r]",					5,	0,	8	},
		/* user region - :MACHINE_NAME:CRC:CPU:ADDRESS_SPACE:START_ADDRESS:END_ADDRESS:STATUS:(DESCRIPTION) */
	{	">%8[^:\n\r]:%8X:%2X:%2X:%X:%X:%1X:%[^:\n\r]",						7,	0,	0	},
#else
		/* code (new) - >GAME_NAME:TYPE:ADDRESS:DATA:EXTEND_DATA:(DESCRIPTION:COMMENT) */
	{	">%8[^:\n\r]:%8X:%X:%8[^:\n\r]:%8[^:\n\r]:%[^:\n\r]:%[^:\n\r]",		5,	8,	7	},
		/* code (old) - :GAME_NAME:TYPE:ADDRESS:DATA:EXTEND_DATA:(DESCRIPTION:COMMENT) */
	{	":%8s:%8X:%X:%8[^:\n\r]:%8[^:\n\r]:%[^:\n\r]:%[^:\n\r]",			5,	8,	0	},
		/* code (older) - :GAME_NAME:CPU:ADDRESS:DATA:TYPE:(DESCRIPTION:COMMENT) */
	{	"%8[^:\n\r]:%d:%X:%X:%d:%[^:\n\r]:%[^:\n\r]",						4,	0,	7	},
		/* user region - :GAME_NAME:CPU:ADDRESS_SPACE:START_ADDRESS:END_ADDRESS:STATUS:(DESCRIPTION) */
	{	">%8[^:\n\r]:%2X:%2X:%X:%X:%1X:%[^:\n\r]",							6,	0,	0	},
#endif	/* MESS */
	/* end of table */
	{	0	}
};

/***************************************************************************
    EXPORTED VARIABLES
***************************************************************************/

static const char			* cheatfile = NULL;

/***************************************************************************
    GLOBAL VARIABLES
***************************************************************************/

static emu_timer			* periodic_timer;

static CheatEntry			* cheatList = NULL;
static INT32				cheatListLength = 0;		// NOTE : 0 means no cheat entry and error

static WatchInfo			* watchList = NULL;
static INT32				watchListLength = 0;

static search_info			*search_list = NULL;
static INT32				search_list_length = 0;
static INT32				current_search_idx = 0;

static cpu_region_info		cpu_info_list[MAX_CPU];
static cpu_region_info		region_info_list[REGION_LIST_LENGTH];

static int					foundCheatDatabase = 0;
static int					cheatsDisabled = 0;
static int					watchesDisabled = 0;

static int					fullMenuPageHeight;
static int					visible_items;

static char					mainDatabaseName[CHEAT_FILENAME_MAX_LEN + 1];

static menu_string_list		menu_strings;

static cheat_menu_item_info	* menu_item_info;
static INT32				menuItemInfoLength = 0;

static UINT8				editActive;
static INT8					editCursor;

static INT8					stack_index;
static cheat_menu_stack		menu_stack[CHEAT_MENU_DEPTH];

static int					cheatVariable[VARIABLE_MAX_ARRAY] = { 0 };

static UINT32				cheatOptions;
static UINT32				driverSpecifiedFlag;
static UINT8				messageType;
static UINT8				messageTimer;

static cpu_region_info raw_cpu_info =
{
	0,			/* type */
	8,			/* data_bits */
	8,			/* address_bits */
	1,			/* address_chars_needed */
	CPU_IS_BE,	/* endianness */
	0,			/* address_shift */
};

static const int kSearchByteIncrementTable[] =
{
	1,
	2,
	3,
	4,
	1
};

static const int kSearchByteStep[] =
{
	1,
	2,
	1,
	4,
	1
};

static const int	kSearchByteDigitsTable[] =
{
	2,
	4,
	6,
	8,
	1
};

static const int	kSearchByteDecDigitsTable[] =
{
	3,
	5,
	8,
	10,
	1
};

static const UINT32 kSearchByteMaskTable[] =
{
	0x000000FF,
	0x0000FFFF,
	0x00FFFFFF,
	0xFFFFFFFF,
	0x00000001
};

static const UINT32	kSearchByteSignBitTable[] =
{
	0x00000080,
	0x00008000,
	0x00800000,
	0x80000000,
	0x00000000
};

static const UINT32 kSearchByteUnsignedMaskTable[] =
{
	0x0000007F,
	0x00007FFF,
	0x007FFFFF,
	0x7FFFFFFF,
	0x00000001
};

static const UINT32	kCheatSizeMaskTable[] =
{
	0x000000FF,
	0x0000FFFF,
	0x00FFFFFF,
	0xFFFFFFFF
};

static const UINT32	kCheatSizeDigitsTable[] =
{
	2,
	4,
	6,
	8
};

static const int	kByteConversionTable[] =
{
	kSearchSize_8Bit,
	kSearchSize_16Bit,
	kSearchSize_24Bit,
	kSearchSize_32Bit,
	kSearchSize_32Bit
};

static const int	kWatchSizeConversionTable[] =
{
	kSearchSize_8Bit,
	kSearchSize_16Bit,
	kSearchSize_24Bit,
	kSearchSize_32Bit,
	kSearchSize_8Bit
};

static const int	kSearchOperandNeedsInit[] =
{
	0,
	1,
	1,
	0
};

static const UINT32 kPrefillValueTable[] =
{
	0x00,
	0xFF,
	0x00,
	0x01
};

static const UINT32 kIncrementValueTable[] =
{
	0x00000001,
	0x00000010,
	0x00000100,
	0x00001000,
	0x00010000,
	0x00100000,
	0x01000000,
	0x10000000
};

static const UINT32 kIncrementMaskTable[] =
{
	0x0000000F,
	0x000000F0,
	0x00000F00,
	0x0000F000,
	0x000F0000,
	0x00F00000,
	0x0F000000,
	0xF0000000
};

static const UINT32 kIncrementDecTable[] =
{
	0x00000009,
	0x00000090,
	0x00000900,
	0x00009000,
	0x00090000,
	0x00900000,
	0x09000000,
	0x90000000
};

static const char *const kRegionNames[] = {
	"INVALID",
	"CPU1",		"CPU2",		"CPU3",		"CPU4",		"CPU5",		"CPU6",		"CPU7",		"CPU8",		// 01-08    [01-08] : CPU
	"GFX1",		"GFX2",		"GFX3",		"GFX4",		"GFX5",		"GFX6",		"GFX7",		"GFX8",		// 09-16    [08-10] : GFX
	"PROMS",																						// 17       [11]    : PROMS
	"SOUND1",	"SOUND2",	"SOUND3",	"SOUND4",	"SOUND5",	"SOUND6",	"SOUND7",	"SOUND8",	// 18-25    [12-19] : SOUND
	"USER1",	"USER2",	"USER3",	"USER4",	"USER5",	"USER6",	"USER7",	"USER8",	// 26-45    [1A-2D] : USER
	/* USER9 - PLDS are undefined in old format */
	"USER9",	"USER10",	"USER11",	"USER12",	"USER13",	"USER14",	"USER15",	"USER16",
	"USER17",	"USER18",	"USER19",	"USER20",
	"DISKS",																						// 46       [2E]    : DISKS
	"PLDS" };																						// 47       [2F]    : PLDS

static const char *const kNumbersTable[] = {
	"0",	"1",	"2",	"3",	"4",	"5",	"6",	"7",
	"8",	"9",	"10",	"11",	"12",	"13",	"14",	"15",
	"16",	"17",	"18",	"19",	"20",	"21",	"22",	"23",
	"24",	"25",	"26",	"27",	"28",	"29",	"30",	"31" };

static const char *const kByteSizeStringList[] =
{
	"8 Bit",
	"16 Bit",
	"24 Bit",
	"32 Bit"
};

static const char *const kWatchLabelStringList[] =
{
	"None",
	"Address",
	"String"
};

static const char *const kWatchDisplayTypeStringList[] =
{
	"Hex",
	"Decimal",
	"Binary",
	"ASCII"
};

/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

static TIMER_CALLBACK( cheat_periodic );
static void 	cheat_exit(running_machine *machine);

/********** SPECIAL KEY HANDLING **********/
static int		ShiftKeyPressed(void);
static int		ControlKeyPressed(void);
static int		AltKeyPressed(void);

static int		ui_pressed_repeat_throttle(running_machine *machine, int code, int base_speed);

/********** KEY INPUT **********/
static int		read_hex_input(void);
#if 0
static char		*DoDynamicEditTextField(char * buf);
#endif
static void		DoStaticEditTextField(char * buf, int size);
static UINT32	do_edit_hex_field(running_machine *machine, UINT32 data);
static UINT32	DoEditHexFieldSigned(UINT32 data, UINT32 mask);
static INT32	DoEditDecField(INT32 data, INT32 min, INT32 max);
static UINT32	DoIncrementHexField(UINT32 data, UINT8 digits);
static UINT32	DoDecrementHexField(UINT32 data, UINT8 digits);
static UINT32	DoIncrementHexFieldSigned(UINT32 data, UINT8 digits, UINT8 bytes);
static UINT32	DoDecrementHexFieldSigned(UINT32 data, UINT8 digits, UINT8 bytes);
static UINT32	DoIncrementDecField(UINT32 data, UINT8 digits);
static UINT32	DoDecrementDecField(UINT32 data, UINT8 digits);

/********** VALUE CONVERTER **********/
static UINT32	BCDToDecimal(UINT32 value);
static UINT32	DecimalToBCD(UINT32 value);

/********** STRINGS **********/
static void		rebuild_string_tables(void);
static void		request_strings(UINT32 length, UINT32 num_strings, UINT32 main_string_length, UINT32 sub_string_length);
static void		init_string_table(void);
static void		free_string_table(void);

static char		*create_string_copy(char *buf);

/********** MENU EXPORTER **********/
static void		old_style_menu(const char **items, const char **sub_items, char *flag, int selected, int arrowize_subitem);

/********** MENU STACK **********/
static void		init_cheat_menu_stack(void);
static void		cheat_menu_stack_push(cheat_menu_handler new_handler, cheat_menu_handler ret_handler, int selection);
static void		cheat_menu_stack_pop(void);
static void		free_cheat_menu_stack(void);

/********** ADDITIONAL MENU FOR CHEAT **********/
static int		user_select_value_menu(running_machine *machine, cheat_menu_stack *menu);
static int		user_select_label_menu(running_machine *machine, cheat_menu_stack *menu);
static int		comment_menu(running_machine *machine, cheat_menu_stack *menu);
static int		extend_comment_menu(running_machine *machine, cheat_menu_stack *menu);

/********** CHEAT MENU **********/
static int		cheat_main_menu(running_machine *machine, cheat_menu_stack *menu);
static int		enable_disable_cheat_menu(running_machine *machine, cheat_menu_stack *menu);
static int		add_edit_cheat_menu(running_machine *machine, cheat_menu_stack *menu);
static int		command_add_edit_menu(running_machine *machine, cheat_menu_stack *menu);
static int		edit_cheat_menu(running_machine *machine, cheat_menu_stack *menu);
static int		view_cheat_menu(running_machine *machine, cheat_menu_stack *menu);
static int		analyse_cheat_menu(running_machine *machine, cheat_menu_stack *menu);

static int		search_minimum_menu(running_machine *machine, cheat_menu_stack *menu);
static int		search_standard_menu(running_machine *machine, cheat_menu_stack *menu);
static int		search_advanced_menu(running_machine *machine, cheat_menu_stack *menu);
static int		select_search_region_menu(running_machine *machine, cheat_menu_stack *menu);
static int		view_search_result_menu(running_machine *machine, cheat_menu_stack *menu);

static int		choose_watch_menu(running_machine *machine, cheat_menu_stack *menu);
static int		command_watch_menu(running_machine *machine, cheat_menu_stack *menu);
static int		edit_watch_menu(running_machine *machine, cheat_menu_stack *menu);

static int		select_option_menu(running_machine *machine, cheat_menu_stack *menu);
static int		select_search_menu(running_machine *machine, cheat_menu_stack *menu);

#ifdef MAME_DEBUG
static int		check_activation_key_code_menu(running_machine *machine, cheat_menu_stack *menu);
static int		view_cpu_region_info_list_menu(running_machine *machine, cheat_menu_stack *menu);
static int		debug_cheat_menu(running_machine *machine, cheat_menu_stack *menu);
#endif

/********** ENTRY LIST **********/
static void		ResizeCheatList(UINT32 newLength);
static void		ResizeCheatListNoDispose(UINT32 newLength);

static void		AddCheatBefore(UINT32 idx);
static void		DeleteCheatAt(UINT32 idx);

static void		DisposeCheat(CheatEntry * entry);
static CheatEntry
				*GetNewCheat(void);

/********** ACTION LIST **********/
static void		ResizeCheatActionList(CheatEntry * entry, UINT32 newLength);
#if 0
static void		ResizeCheatActionListNoDispose(CheatEntry * entry, UINT32 newLength);

static void		AddActionBefore(CheatEntry * entry, UINT32 idx);
static void		DeleteActionAt(CheatEntry * entry, UINT32 idx);
#endif
static void		DisposeAction(CheatAction * action);

/********** WATCH LIST **********/
static void		InitWatch(WatchInfo * info, UINT32 idx);
static void		ResizeWatchList(UINT32 newLength);
static void		ResizeWatchListNoDispose(UINT32 newLength);

static void		AddWatchBefore(UINT32 idx);
static void		DeleteWatchAt(UINT32 idx);
static void		DisposeWatch(WatchInfo * watch);
static WatchInfo
				*GetUnusedWatch(void);

static void		AddCheatFromWatch(WatchInfo * watch);
static void		SetupCheatFromWatchAsWatch(CheatEntry * entry, WatchInfo * watch);

static void		ResetWatch(WatchInfo * watch);

/********** SEARCH LIST **********/
static void		resize_search_list(UINT32 newLength);
#if 0
static void		resize_search_list_no_dispose(UINT32 new_length);
static void		add_search_before(UINT32 idx);
static void		delete_search_at(UINT32 idx);
#endif
static void		init_search(search_info *info);

static void		dispose_search_reigons(search_info *info);
static void		dispose_search(search_info *info);
static search_info
				*get_current_search(void);

static void		fill_buffer_from_region(search_region *region, UINT8 *buf);
static UINT32	read_region_data(search_region *region, UINT32 offset, UINT8 size, UINT8 swap);

static void		backup_search(search_info *info);
static void		restore_search_backup(search_info *info);
static void		backup_region(search_region *region);
static void		restore_region_backup(search_region *region);
static UINT8	default_enable_region(running_machine *machine, search_region *region, search_info *info);
static void		set_search_region_default_name(search_region *region);
static void		allocate_search_regions(search_info *info);
static void		build_search_regions(running_machine *machine, search_info *info);

/********** LOADER **********/
static int		ConvertOldCode(int code, int cpu, int * data, int * extendData);
static int		ConvertToNewCode(CheatAction * action);
static void		HandleLocalCommandCheat(running_machine *machine, int cpu, int type, int address, int data, UINT8 format);

static void		LoadCheatOption(char * fileName);
static void		LoadCheatCode(running_machine *machine, char * fileName);
static void		LoadUserDefinedSearchRegion(running_machine *machine, char * fileName);
static void		LoadCheatDatabase(running_machine *machine, UINT8 flags);

static void		DisposeCheatDatabase(void);
static void		ReloadCheatDatabase(running_machine *machine);

/********** SAVER **********/
static void		SaveCheatCode(running_machine *machine, CheatEntry * entry);
static void		SaveActivationKey(running_machine *machine, CheatEntry * entry, int entryIndex);
static void		SavePreEnable(running_machine *machine, CheatEntry * entry, int entryIndex);
static void		SaveCheatOptions(void);
static void		SaveDescription(running_machine *machine);
static void		DoAutoSaveCheats(running_machine *machine);

/********** CODE ADDITION **********/
static void		AddCheatFromResult(search_info *search, search_region *region, UINT32 address);
static void		AddCheatFromFirstResult(search_info *search);
static void		AddWatchFromResult(search_info *search, search_region *region, UINT32 address);

/********** SEARCH **********/
static UINT32	search_sign_extend(search_info *search, UINT32 value);
static UINT32	read_search_operand(UINT8 type, search_info *search, search_region *region, UINT32 address);
static UINT32	read_search_operand_bit(UINT8 type, search_info *search, search_region *region, UINT32 address);
static UINT8	do_search_comparison(search_info *search, UINT32 lhs, UINT32 rhs);
static UINT32	do_search_comparison_bit(search_info *search, UINT32 lhs, UINT32 rhs);
#if 0
static UINT8  is_region_offset_valid(search_info *search, search_region *region, UINT32 offset);
#else
#define is_region_offset_valid is_region_offset_valid_bit /* ????? */
#endif
static UINT8	is_region_offset_valid_bit(search_info *search, search_region *region, UINT32 offset);
static void		invalidate_region_offset(search_info *search, search_region *region, UINT32 offset);
static void		invalidate_region_offset_bit(search_info *search, search_region *region, UINT32 offset, UINT32 invalidate);
static void		invalidate_entire_region(search_info *search, search_region *region);

static void		init_new_search(search_info *search);
static void		update_search(search_info *search);

static void		do_search(search_info *search);

/********** MEMORY ACCESSOR **********/
static UINT8 **	LookupHandlerMemory(UINT8 cpu, UINT32 address, UINT32 * outRelativeAddress);
static UINT8 **	GetMemoryRegionBasePointer(UINT8 cpu, UINT8 space, UINT32 address);

static UINT32	DoCPURead(UINT8 cpu, UINT32 address, UINT8 bytes, UINT8 swap);
static UINT32	DoMemoryRead(UINT8 * buf, UINT32 address, UINT8 bytes, UINT8 swap, cpu_region_info *info);
static void		DoCPUWrite(UINT32 data, UINT8 cpu, UINT32 address, UINT8 bytes, UINT8 swap);
static void		DoMemoryWrite(UINT32 data, UINT8 * buf, UINT32 address, UINT8 bytes, UINT8 swap, cpu_region_info *info);

static UINT32	ReadData(CheatAction * action);
static void		WriteData(CheatAction * action, UINT32 data);

/********** WATCH **********/
static void		WatchCheatEntry(CheatEntry * entry, UINT8 associate);
static void		AddActionWatch(CheatAction * action, CheatEntry * entry);
static void		RemoveAssociatedWatches(CheatEntry * entry);

/********** ACTIVE/DEACTIVE ENTRY **********/
static void		ResetAction(CheatAction * action);
static void		ActivateCheat(CheatEntry * entry);
static void		DeactivateCheat(CheatEntry * entry);
static void		TempDeactivateCheat(CheatEntry * entry);

/********** OPERATION CORE **********/
static void		cheat_periodicOperation(CheatAction * action);
static UINT8	cheat_periodicCondition(CheatAction * action);
static int		cheat_periodicAction(running_machine *machine, CheatAction * action, int selection);
static void		cheat_periodicEntry(running_machine *machine, CheatEntry * entry);

/********** CONFIGURE ENTRY **********/
static void		UpdateAllCheatInfo(void);
static void		UpdateCheatInfo(CheatEntry * entry, UINT8 isLoadTime);
static UINT32	AnalyseCodeFormat(CheatEntry * entry, CheatAction * action);
static void		CheckCodeFormat(CheatEntry * entry);
static void		BuildLabelIndexTable(CheatEntry *entry);
static void		SetLayerIndex(void);

/********** OTHER STUFF **********/
static void		DisplayCheatMessage(void);
static UINT8	get_address_length(UINT8 region);
static char *	get_region_name(UINT8 region);
static void		build_cpu_region_info_list(running_machine *machine);

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

/*-------------
  GetCPUInfo
-------------*/

INLINE cpu_region_info *get_cpu_info(UINT8 cpu)
{
	if(VALID_CPU(EXTRACT_FIELD(cpu, CPUIndex)))
		return &cpu_info_list[cpu];

	return NULL;
}

/*-------------------
  GetRegionCPUInfo
-------------------*/

INLINE cpu_region_info *get_region_info(UINT8 region)
{
	if(region >= REGION_CPU1 && region < REGION_MAX)
		return &region_info_list[region - REGION_INVALID];

	return NULL;
}

/*-------------------------
  get_cpu_or_region_info
-------------------------*/

INLINE cpu_region_info *get_cpu_or_region_info(UINT8 cpu_region)
{
	if(TEST_FIELD(cpu_region, RegionFlag))
		return get_region_info(cpu_region);
	else
		return get_cpu_info(cpu_region);
}

/*-----------------
  cpu_needs_swap
-----------------*/

INLINE UINT8 cpu_needs_swap(UINT8 cpu)
{
	return cpu_info_list[cpu].endianness | 1;
}

/*--------------------
  region_needs_swap
--------------------*/

INLINE UINT8 region_needs_swap(UINT8 region)
{
	cpu_region_info *temp = get_region_info(region);

	if(temp)
		return temp->endianness | 1;

	return 0;
}

/*--------------
  SwapAddress
--------------*/

INLINE UINT32 SwapAddress(UINT32 address, UINT8 dataSize, cpu_region_info *info)
{
	switch(info->data_bits)
	{
		case 16:
			if(info->endianness == CPU_IS_BE)
				return BYTE_XOR_BE(address);
			else
				return BYTE_XOR_LE(address);

		case 32:
			if(info->endianness == CPU_IS_BE)
				return BYTE4_XOR_BE(address);
			else
				return BYTE4_XOR_LE(address);
	}

	return address;
}

/*-------------------
  IsAddressInRange
-------------------*/

INLINE int IsAddressInRange(CheatAction * action, UINT32 length)
{
	return ((action->address + EXTRACT_FIELD(action->type, AddressSize) + 1) <= length);
}

/*----------
  DoShift
----------*/

INLINE UINT32 DoShift(UINT32 input, INT8 shift)
{
	if(shift > 0)
		return input >> shift;
	else
		return input << -shift;
}

/***************************************************************************
    KEY HANDLER
***************************************************************************/

/*--------------------------------------------------------------
  special key handler - check pressing shift, ctrl or alt key
--------------------------------------------------------------*/

static int ShiftKeyPressed(void)
{
	return (input_code_pressed(KEYCODE_LSHIFT) || input_code_pressed(KEYCODE_RSHIFT));
}

static int ControlKeyPressed(void)
{
	return (input_code_pressed(KEYCODE_LCONTROL) || input_code_pressed(KEYCODE_RCONTROL));
}

static int AltKeyPressed(void)
{
	return (input_code_pressed(KEYCODE_LALT) || input_code_pressed(KEYCODE_RALT));
}

/*---------------------------------------------------------------------------------------------------------------------
  ReadKeyAsync - dirty hack until osd_readkey_unicode is supported in MAMEW re-implementation of osd_readkey_unicode
---------------------------------------------------------------------------------------------------------------------*/

#if 1

#if OSD_READKEY_KLUDGE

static int ReadKeyAsync(int flush)
{
	int	code;

	if(flush)		// check key input
	{
		while(input_code_poll_keyboard_switches(TRUE) != INPUT_CODE_INVALID) ;

		return 0;
	}

	while(1)		// check pressed key
	{
		code = input_code_poll_keyboard_switches(FALSE);

		if(code == INPUT_CODE_INVALID)
		{
			return 0;
		}
		else if((code >= KEYCODE_A) && (code <= KEYCODE_Z))
		{
			if(ShiftKeyPressed())
			{
				return 'A' + (code - KEYCODE_A);
			}
			else
			{
				return 'a' + (code - KEYCODE_A);
			}
		}
		else if((code >= KEYCODE_0) && (code <= KEYCODE_9))
		{
			if(ShiftKeyPressed())
			{
				return ")!@#$%^&*("[code - KEYCODE_0];
			}
			else
			{
				return '0' + (code - KEYCODE_0);
			}
		}
		else if((code >= KEYCODE_0_PAD) && (code <= KEYCODE_9_PAD))
		{
			return '0' + (code - KEYCODE_0_PAD);
		}
		else if(code == KEYCODE_TILDE)
		{
			if(ShiftKeyPressed())
			{
				return '~';
			}
			else
			{
				return '`';
			}
		}
		else if(code == KEYCODE_MINUS)
		{
			if(ShiftKeyPressed())
			{
				return '_';
			}
			else
			{
				return '-';
			}
		}
		else if(code == KEYCODE_EQUALS)
		{
			if(ShiftKeyPressed())
			{
				return '+';
			}
			else
			{
				return '=';
			}
		}
		else if(code == KEYCODE_BACKSPACE)
		{
			return 0x08;
		}
		else if(code == KEYCODE_OPENBRACE)
		{
			if(ShiftKeyPressed())
			{
				return '{';
			}
			else
			{
				return '[';
			}
		}
		else if(code == KEYCODE_CLOSEBRACE)
		{
			if(ShiftKeyPressed())
			{
				return '}';
			}
			else
			{
				return ']';
			}
		}
		else if(code == KEYCODE_COLON)
		{
			if(ShiftKeyPressed())
			{
				return ':';
			}
			else
			{
				return ';';
			}
		}
		else if(code == KEYCODE_QUOTE)
		{
			if(ShiftKeyPressed())
			{
				return '\"';
			}
			else
			{
				return '\'';
			}
		}
		else if(code == KEYCODE_BACKSLASH)
		{
			if(ShiftKeyPressed())
			{
				return '|';
			}
			else
			{
				return '\\';
			}
		}
		else if(code == KEYCODE_COMMA)
		{
			if(ShiftKeyPressed())
			{
				return '<';
			}
			else
			{
				return ',';
			}
		}
		else if(code == KEYCODE_STOP)
		{
			if(ShiftKeyPressed())
			{
				return '>';
			}
			else
			{
				return '.';
			}
		}
		else if(code == KEYCODE_SLASH)
		{
			if(ShiftKeyPressed())
			{
				return '?';
			}
			else
			{
				return '/';
			}
		}
		else if(code == KEYCODE_SLASH_PAD)
		{
			return '/';
		}
		else if(code == KEYCODE_ASTERISK)
		{
			return '*';
		}
		else if(code == KEYCODE_MINUS_PAD)
		{
			return '-';
		}
		else if(code == KEYCODE_PLUS_PAD)
		{
			return '+';
		}
		else if(code == KEYCODE_SPACE)
		{
			return ' ';
		}
	}
}

#define osd_readkey_unicode ReadKeyAsync

#endif

#endif

/*---------------------------------------------------
  ui_pressed_repeat_throttle - key repeat handling
---------------------------------------------------*/

static int ui_pressed_repeat_throttle(running_machine *machine, int code, int base_speed)
{
	int pressed = 0;
	const int delay_ramp_timer = 10;
	static int last_code = -1;
	static int last_speed = -1;
	static int increment_timer = 0;

	if(input_type_pressed(machine, code, 0))
	{
		if(last_code != code)
		{
			/* pressed different key */
			last_code = code;
			last_speed = base_speed;
			increment_timer = delay_ramp_timer * last_speed;
		}
		else
		{
			/* hold the same key */
			increment_timer--;

			if(increment_timer <= 0)
			{
				increment_timer = delay_ramp_timer * last_speed;

				last_speed /= 2;

				if(last_speed < 1)
					last_speed = 1;

				pressed = 1;
			}
		}
	}
	else
	{
		if(last_code == code)
			last_code = -1;
	}

	return input_ui_pressed_repeat(machine, code, last_speed);
}

/*-----------------------------------
  read_hex_input - check hex input
-----------------------------------*/

static int read_hex_input(void)
{
	int i;

	for(i = 0; i < 10; i++)
	{
		if(input_code_pressed_once(KEYCODE_0 + i))
			return i;
	}

	for(i = 0; i < 10; i++)
	{
		if(input_code_pressed_once(KEYCODE_0_PAD + i))
			return i;
	}

	for(i = 0; i < 6; i++)
	{
		if(input_code_pressed_once(KEYCODE_A + i))
			return i + 10;
	}

	return -1;
}

/*-----------------------------------------------------------------
  DoDynamicEditTextField - edit text field with direct key input
-----------------------------------------------------------------*/
#if 0
static char * DoDynamicEditTextField(char * buf)
{
	char code = osd_readkey_unicode(0) & 0xFF;

	if(code == 0x08)
	{
		/* backspace */
		if(buf)
		{
			size_t	length = strlen(buf);

			if(length > 0)
			{
				buf[length - 1] = 0;

				if(length > 1)
					buf = realloc(buf, length);
				else
				{
					free(buf);

					buf = NULL;
				}
			}
		}
	}
	else
	{
		if(isprint(code))
		{
			if(buf)
			{
				size_t length = strlen(buf);

				buf = realloc(buf, length + 2);

				buf[length] = code;
				buf[length + 1] = 0;
			}
			else
			{
				buf = malloc(2);

				buf[0] = code;
				buf[1] = 0;
			}
		}
	}

	return buf;
}
#endif
/*------------------------
  DoStaticEditTextField
------------------------*/

static void DoStaticEditTextField(char * buf, int size)
{
	char	code = osd_readkey_unicode(0) & 0xFF;
	size_t	length;

	if(!buf)
		return;

	length = strlen(buf);

	if(code == 0x08)
	{
		/* back space */
		if(length > 0)
			buf[length - 1] = 0;
	}
	else if(isprint(code))
	{
		if(length + 1 < size)
		{
			buf[length] = code;
			buf[length + 1] = 0;
		}
	}
}

/*----------------------------------------------------------------------
  do_edit_hex_field - edit hex field with direct key input (unsigned)
----------------------------------------------------------------------*/

static UINT32 do_edit_hex_field(running_machine *machine, UINT32 data)
{
	INT8 key;

	if(input_code_pressed_once(KEYCODE_BACKSPACE))
	{
		/* back space */
		data >>= 4;
		return data;
	}
	else if(input_ui_pressed(machine, IPT_UI_CLEAR))
	{
		/* data clear */
		data = 0;
		return data;
	}

	key = read_hex_input();

	if(key != -1)
	{
		data <<= 4;
		data |= key;
	}

	return data;
}

/*-----------------------------------------------------------------------
  DoEditHexFieldSigned - edit hex field with direct key input (signed)
-----------------------------------------------------------------------*/

static UINT32 DoEditHexFieldSigned(UINT32 data, UINT32 mask)
{
	INT8	key;
	UINT32	isNegative = data & mask;

	if(isNegative)
		data |= mask;

	key = read_hex_input();

	if(key != -1)
	{
		if(isNegative)
			data = (~data) + 1;

		data <<= 4;
		data |= key;

		if(isNegative)
			data = (~data) + 1;
	}
	else
	{
		if(input_code_pressed_once(KEYCODE_MINUS))
			data = (~data) + 1;
	}

	return data;
}

/*--------------------------------------------------------
  DoEditDecField - edit dec field with direct key input
--------------------------------------------------------*/

static INT32 DoEditDecField(INT32 data, INT32 min, INT32 max)
{
	char code = osd_readkey_unicode(0) & 0xFF;

	if((code >= '0') && (code <= '9'))
	{
		data *= 10;
		data += (code - '0');
	}
	else
	{
		if(code == '-')
			data = -data;
		else
		{
			/* backspace */
			if(code == 0x08)
				data /= 10;
		}
	}

	/* adjust value */
	if(data < min)
		data = min;
	if(data > max)
		data = max;

	return data;
}

/*--------------------------------------------------------------------------------
  DoIncrementHexField - increment a specified value in hex field with arrow key
--------------------------------------------------------------------------------*/

static UINT32 DoIncrementHexField(UINT32 data, UINT8 digits)
{
	data =	((data & ~kIncrementMaskTable[digits]) |
			(((data & kIncrementMaskTable[digits]) + kIncrementValueTable[digits]) &
			kIncrementMaskTable[digits]));

	return data;
}

/*--------------------------------------------------------------------------------
  DoDecrementHexField - decrement a specified value in hex field with arrow key
--------------------------------------------------------------------------------*/

static UINT32 DoDecrementHexField(UINT32 data, UINT8 digits)
{
	data =	((data & ~kIncrementMaskTable[digits]) |
			(((data & kIncrementMaskTable[digits]) - kIncrementValueTable[digits]) &
			kIncrementMaskTable[digits]));

	return data;
}

/*--------------------------------------------------------------------------------------
  DoIncrementHexFieldSigned - increment a specified value in hex field with arrow key
--------------------------------------------------------------------------------------*/

static UINT32 DoIncrementHexFieldSigned(UINT32 data, UINT8 digits, UINT8 bytes)
{
	UINT32 newData = data;

	if(data & kSearchByteSignBitTable[bytes])
	{
		/* MINUS */
		newData =	((newData & ~kIncrementMaskTable[digits]) |
					(((newData & kIncrementMaskTable[digits]) - kIncrementValueTable[digits]) &
					kIncrementMaskTable[digits]));

		if(newData & kSearchByteSignBitTable[bytes])
			newData = ((data & ~kIncrementMaskTable[digits]) | (newData & kIncrementMaskTable[digits]));
	}
	else
	{
		/* PLUS */
		newData =	((newData & ~kIncrementMaskTable[digits]) |
					(((newData & kIncrementMaskTable[digits]) + kIncrementValueTable[digits]) &
					kIncrementMaskTable[digits]));

		if(!(newData & kSearchByteSignBitTable[bytes]))
			newData = ((data & ~kIncrementMaskTable[digits]) | (newData & kIncrementMaskTable[digits]));
	}

	return newData;
}

/*--------------------------------------------------------------------------------------
  DoDecrementHexFieldSigned - decrement a specified value in hex field with arrow key
--------------------------------------------------------------------------------------*/

static UINT32 DoDecrementHexFieldSigned(UINT32 data, UINT8 digits, UINT8 bytes)
{
	UINT32 newData = data;

	if(data & kSearchByteSignBitTable[bytes])
	{
		/* MINUS */
		newData =	((newData & ~kIncrementMaskTable[digits]) |
					(((newData & kIncrementMaskTable[digits]) + kIncrementValueTable[digits]) &
					kIncrementMaskTable[digits]));

		if(newData & kSearchByteSignBitTable[bytes])
			newData = ((data & ~kIncrementMaskTable[digits]) | (newData & kIncrementMaskTable[digits]));
	}
	else
	{
		/* PLUS */
		newData =	((newData & ~kIncrementMaskTable[digits]) |
					(((newData & kIncrementMaskTable[digits]) - kIncrementValueTable[digits]) &
					kIncrementMaskTable[digits]));

		if(!(newData & kSearchByteSignBitTable[bytes]))
			newData = ((data & ~kIncrementMaskTable[digits]) | (newData & kIncrementMaskTable[digits]));
	}

	return newData;
}

/*--------------------------------------------------------------------------------
  DoIncrementDecField - increment a specified value in dec field with arrow key
--------------------------------------------------------------------------------*/

static UINT32 DoIncrementDecField(UINT32 data, UINT8 digits)
{
	UINT32 value = data & kIncrementMaskTable[digits];

	if(value >= kIncrementDecTable[digits])
		value = 0;
	else
		value += kIncrementValueTable[digits];

	return ((data & ~kIncrementMaskTable[digits]) | value);
}

/*--------------------------------------------------------------------------------
  DoDecrementDecField - increment a specified value in dec field with arrow key
--------------------------------------------------------------------------------*/

static UINT32 DoDecrementDecField(UINT32 data, UINT8 digits)
{
	UINT32 value = data & kIncrementMaskTable[digits];

	if(!value)
		value = kIncrementDecTable[digits];
	else
		value -= kIncrementValueTable[digits];

	return ((data & ~kIncrementMaskTable[digits]) | value);
}

/***************************************************************************
    INITIALIZATION
***************************************************************************/

/*---------------------------------------
  cheat_init - initialize cheat system
---------------------------------------*/

void cheat_init(running_machine *machine)
{
	/* initialize lists */
	cheatList			= NULL;
	cheatListLength		= 0;

	watchList			= NULL;
	watchListLength		= 0;

	search_list = NULL;
	search_list_length = 0;

#ifdef MESS
	InitMessCheats(machine);
#endif

	/* initialize flags */
	current_search_idx	= 0;
	foundCheatDatabase	= 0;
	cheatsDisabled		= 0;
	watchesDisabled		= 0;
	editActive			= 0;
	editCursor			= 0;
	cheatOptions		= DEFAULT_CHEAT_OPTIONS;
	driverSpecifiedFlag	= 0;

	fullMenuPageHeight = visible_items = floor(1.0f / ui_get_line_height()) - 1;

	/* initialize CPU/Region info for cheat system */
	build_cpu_region_info_list(machine);

	/* set cheat list from database */
	LoadCheatDatabase(machine, kLoadFlag_CheatOption | kLoadFlag_CheatCode);

	/* set default search and watch lists */
	resize_search_list(1);
	ResizeWatchList(fullMenuPageHeight - 1);

	/* build search region */
	{
		search_info *info = get_current_search();

		/* attemp to load user region */
		LoadCheatDatabase(machine, kLoadFlag_UserRegion);

		/* if no user region or fail to load the database, attempt to build default region */
		if(info->region_list_length)
			info->search_speed = ksearch_speed_UserDefined;
		else
		{
			info->search_speed = ksearch_speed_VerySlow;
			build_search_regions(machine, info);
		}

		/* memory allocation for search region */
		allocate_search_regions(info);
	}

	/* initialize string table */
	init_string_table();

	/* initialize cheat menu stack */
	init_cheat_menu_stack();

	{
		astring * sourceName;

		sourceName = core_filename_extract_base(astring_alloc(), machine->gamedrv->source_file, TRUE);

		if(!strcmp(astring_c(sourceName), "neodrvr"))
			driverSpecifiedFlag |= 1;
		else if(!strcmp(astring_c(sourceName), "cps2"))
			driverSpecifiedFlag |= 2;
		else if(!strcmp(astring_c(sourceName), "cps3"))
			driverSpecifiedFlag |= 4;

		astring_free(sourceName);
	}

	/* NOTE : disable all cheat messages in initializing so that message parameters should be initialized here */
	messageType		= 0;
	messageTimer	= 0;

	periodic_timer = timer_alloc(cheat_periodic, NULL);
	timer_adjust_periodic(periodic_timer, video_screen_get_frame_period(machine->primary_screen), 0, video_screen_get_frame_period(machine->primary_screen));

	add_exit_callback(machine, cheat_exit);
}

/*---------------------------------
  cheat_exit - free cheat system
---------------------------------*/

static void cheat_exit(running_machine *machine)
{
	int i;

	/* save all cheats automatically if needed */
	if(TEST_FIELD(cheatOptions, AutoSaveEnabled))
		DoAutoSaveCheats(machine);

	/* free database */
	DisposeCheatDatabase();

	/* free watch lists */
	if(watchList)
	{
		for(i = 0; i < watchListLength; i++)
			DisposeWatch(&watchList[i]);

		free(watchList);

		watchList = NULL;
	}

	/* free search lists */
	if(search_list)
	{
		for(i = 0; i < search_list_length; i++)
			dispose_search(&search_list[i]);

		free(search_list);

		search_list = NULL;
	}

	/* free string table */
	free_string_table();

	/* free cheat menu stack */
	free_cheat_menu_stack();

	free(menu_item_info);
	menu_item_info = NULL;

#ifdef MESS
	StopMessCheats();
#endif

	/* free flags */
	cheatListLength		= 0;
	watchListLength		= 0;
	search_list_length	= 0;
	current_search_idx	= 0;
	foundCheatDatabase	= 0;
	cheatsDisabled		= 0;
	watchesDisabled		= 0;
	mainDatabaseName[0]	= 0;
	menuItemInfoLength	= 0;
	cheatVariable[0]	= 0;
	cheatOptions		= 0;
	driverSpecifiedFlag	= 0;
	messageType			= 0;
	messageTimer		= 0;
}

/*----------------------------------
  cheat_menu - handle cheat menus
----------------------------------*/

int cheat_menu(running_machine *machine, int selection)
{
	cheat_menu_stack *menu = &menu_stack[stack_index];

	/* handle cheat message */
	DisplayCheatMessage();

	/* handle cheat menu */
	selection = (*menu->handler)(machine, menu);

	if(selection == 0)
	{
		if(stack_index)
			/* return previous "cheat" menu */
			cheat_menu_stack_pop();
		else
			/* return MAME general menu */
			return selection;
	}

	return selection + 1;
}

/*----------------------------------------
  BCDToDecimal - convert a value to hex
----------------------------------------*/

static UINT32 BCDToDecimal(UINT32 value)
{
	int		i;
	UINT32	accumulator	= 0;
	UINT32	multiplier	= 1;

	for(i = 0; i < 8; i++)
	{
		accumulator += (value & 0xF) * multiplier;

		multiplier *= 10;
		value >>= 4;
	}

	return accumulator;
}

/*----------------------------------------
  DecimalToBCD - convert a value to dec
----------------------------------------*/

static UINT32 DecimalToBCD(UINT32 value)
{
	int		i;
	UINT32	accumulator	= 0;
	UINT32	divisor		= 10;

	for(i = 0; i < 8; i++)
	{
		UINT32	temp;

		temp = value % divisor;
		value -= temp;
		temp /= divisor / 10;

		accumulator += temp << (i * 4);

		divisor *= 10;
	}

	return accumulator;
}

/*-----------------------------------------------------------------
  rebuild_string_tables - memory allocation for menu string list
-----------------------------------------------------------------*/

static void rebuild_string_tables(void)
{
	UINT32 i;
	UINT32 storage_needed;
	char *traverse;

	/* calculate length of total buffer */
	storage_needed = (menu_strings.main_string_length + menu_strings.sub_string_length) * menu_strings.num_strings;

	/* allocate memory for all items */
	menu_strings.main_list =	(const char **)	realloc((char *)	menu_strings.main_list,		sizeof(char *)	* menu_strings.length);
	menu_strings.sub_list =		(const char **)	realloc((char *)	menu_strings.sub_list,		sizeof(char *)	* menu_strings.length);
	menu_strings.flag_list =					realloc(			menu_strings.flag_list,		sizeof(char)	* menu_strings.length);
	menu_strings.main_strings =					realloc(			menu_strings.main_strings,	sizeof(char *)	* menu_strings.num_strings);
	menu_strings.sub_strings =					realloc(			menu_strings.sub_strings,	sizeof(char *)	* menu_strings.num_strings);
	menu_strings.buf =							realloc(			menu_strings.buf,			sizeof(char)	* storage_needed);

	if(	(menu_strings.main_list == NULL && menu_strings.length) ||
		(menu_strings.sub_list == NULL && menu_strings.length) ||
		(menu_strings.flag_list == NULL && menu_strings.length) ||
		(menu_strings.main_strings == NULL && menu_strings.num_strings) ||
		(menu_strings.sub_strings == NULL && menu_strings.num_strings) ||
		(menu_strings.buf == NULL && storage_needed))
	{
		fatalerror(	"cheat: [string table] memory allocation error\n"
					"	length =			%.8X\n"
					"	num_strings =		%.8X\n"
					"	main_stringLength =	%.8X\n"
					"	sub_string_length =	%.8X\n"
					"%p %p %p %p %p %p\n",
					menu_strings.length,
					menu_strings.num_strings,
					menu_strings.main_string_length,
					menu_strings.sub_string_length,

					menu_strings.main_list,
					menu_strings.sub_list,
					menu_strings.flag_list,
					menu_strings.main_strings,
					menu_strings.sub_strings,
					menu_strings.buf);
	}

	traverse = menu_strings.buf;

	/* allocate string buffer to list */
	for(i = 0; i < menu_strings.num_strings; i++)
	{
		menu_strings.main_strings[i] = traverse;
		traverse += menu_strings.main_string_length;
		menu_strings.sub_strings[i] = traverse;
		traverse += menu_strings.sub_string_length;
	}
}

/*------------------------------------------------------------
  request_strings - check string list and rebuild if needed
------------------------------------------------------------*/

static void request_strings(UINT32 length, UINT32 num_strings, UINT32 main_string_length, UINT32 sub_string_length)
{
	UINT8 changed = 0;

	/* total items */
	if(menu_strings.length < length)
	{
		menu_strings.length = length;
		changed = 1;
	}

	/* total buffer items */
	if(menu_strings.num_strings < num_strings)
	{
		menu_strings.num_strings = num_strings;
		changed = 1;
	}

	/* length of buffer 1*/
	if(menu_strings.main_string_length < main_string_length)
	{
		menu_strings.main_string_length = main_string_length;
		changed = 1;
	}

	/* length of buffer 2 */
	if(menu_strings.sub_string_length < sub_string_length)
	{
		menu_strings.sub_string_length = sub_string_length;
		changed = 1;
	}

	if(changed)
		rebuild_string_tables();
}

/*----------------------------------------------
  init_string_table - initialize string table
----------------------------------------------*/

static void init_string_table(void)
{
	memset(&menu_strings, 0, sizeof(menu_string_list));
}

/*----------------------------------------
  free_string_table - free string table
----------------------------------------*/

static void free_string_table(void)
{
	free((char *)	menu_strings.main_list);
	free((char *)	menu_strings.sub_list);
	free(			menu_strings.flag_list);
	free(			menu_strings.main_strings);
	free(			menu_strings.sub_strings);
	free(			menu_strings.buf);

	memset(&menu_strings, 0, sizeof(menu_string_list));
}

/*-----------------------------------
  create_string_copy - copy stirng
-----------------------------------*/

static char *create_string_copy(char *buf)
{
	char *temp = NULL;

	if(buf)
	{
		/* allocate memory */
		size_t length = strlen(buf) + 1;
		temp = malloc_or_die(length);

		/* copy memory for string */
		if(temp)
			memcpy(temp, buf, length);
	}

	return temp;
}

/*--------------------------------------------------
  old_style_menu - export menu items to draw menu
--------------------------------------------------*/

static void old_style_menu(const char **items, const char **sub_items, char *flag, int selected, int arrowize_subitem)
{
	int menu_items;
	static ui_menu_item item_list[1000];

	for(menu_items = 0; items[menu_items]; menu_items++)
	{
		item_list[menu_items].text = items[menu_items];
		item_list[menu_items].subtext = sub_items ? sub_items[menu_items] : NULL;
		item_list[menu_items].flags = 0;

		/* set a highlight for sub-item */
		if(flag && flag[menu_items])
			item_list[menu_items].flags |= MENU_FLAG_INVERT;

		/* set an arrow for sub-item */
		if(menu_items == selected)
		{
			if (arrowize_subitem & 1)
				item_list[menu_items].flags |= MENU_FLAG_LEFT_ARROW;
			if (arrowize_subitem & 2)
				item_list[menu_items].flags |= MENU_FLAG_RIGHT_ARROW;
		}
	}

	visible_items = ui_menu_draw(item_list, menu_items, selected, NULL);
}

/*-------------------------------------------------------
  init_cheat_menu_stack - initilalize cheat menu stack
-------------------------------------------------------*/

static void init_cheat_menu_stack(void)
{
	cheat_menu_stack *menu = &menu_stack[stack_index = 0];

	menu->handler = cheat_main_menu;
	menu->sel = 0;
	menu->pre_sel = 0;
	menu->first_time = 1;
}

/*-----------------------------------------------------
  cheat_menu_stack_push - push a menu onto the stack
-----------------------------------------------------*/

static void cheat_menu_stack_push(cheat_menu_handler new_handler, cheat_menu_handler ret_handler, int selection)
{
	cheat_menu_stack *menu = &menu_stack[++stack_index];

	assert(stack_index < CHEAT_MENU_DEPTH);

	menu->handler = new_handler;
	menu->return_handler = ret_handler;
	menu->sel = 0;
	menu->pre_sel = selection;
	menu->first_time = 1;
	editActive = 0;
}

/*---------------------------------------------------
  cheat_menu_stack_pop - pop a menu from the stack
---------------------------------------------------*/

static void cheat_menu_stack_pop(void)
{
	int i;

	assert(stack_index > 0);

	/* NOTE : cheat menu supports deep-return */
	for(i = stack_index; i > 0; i--)
		if(menu_stack[stack_index].return_handler == menu_stack[i].handler)
			break;

	stack_index = i;
}

/*-----------------------------------------------------
  free_cheat_menu_stack - reset the cheat menu stack
-----------------------------------------------------*/

static void free_cheat_menu_stack(void)
{
	int i;
	cheat_menu_stack *menu;

	for(i = 0; i < CHEAT_MENU_DEPTH; i++)
	{
		menu = &menu_stack[i];

		menu->handler = NULL;
		menu->return_handler = NULL;
		menu->pre_sel = 0;
		menu->first_time = 0;
	}

	stack_index = 0;
}

/*---------------------------------------------------------------
  user_select_value_menu - management for value-selection menu
---------------------------------------------------------------*/

static int user_select_value_menu(running_machine *machine, cheat_menu_stack *menu)
{
	int				i;
	UINT8			isBCD;
	UINT32			min;
	UINT32			max;
	static int		master			= 0;
	static int		displayValue	= -1;
	char			buf[256];
	char *			stringsBuf		= buf;
	CheatEntry *	entry			= &cheatList[menu->pre_sel];
	CheatAction *	action			= NULL;

	/* first setting 1 : search master code of value-selection */
	if(menu->first_time)
	{
		for(i = 0; i < entry->actionListLength; i++)
		{
			/* get value selection master code */
			if(	EXTRACT_FIELD(entry->actionList[i].type, CodeType) == kCodeType_VWrite ||
				EXTRACT_FIELD(entry->actionList[i].type, CodeType) == kCodeType_VRWrite)
			{
				master = i;
				break;
			}
		}
	}

	action = &entry->actionList[master];

	if(TEST_FIELD(action->type, ValueSelectMinimumDisplay) || TEST_FIELD(action->type, ValueSelectMinimum))
		min = 1;
	else
		min = 0;

	max		= action->originalDataField + min;
	isBCD	= EXTRACT_FIELD(action->type, ValueSelectBCD);

	/* first setting 2 : save the value */
	if(menu->first_time)
	{
		displayValue = isBCD ? DecimalToBCD(BCDToDecimal(ReadData(action))) : ReadData(action);

		if(displayValue < min)
			displayValue = min;
		else if(displayValue > max)
			displayValue = max;
		else if(TEST_FIELD(action->type, ValueSelectMinimumDisplay))
			displayValue++;

		menu->first_time = 0;
	}

	/* ##### MENU CONSTRACTION ##### */
	stringsBuf += sprintf(stringsBuf, "\t%s\n\t", entry->name);

	if(editActive)
	{
		for(i = kSearchByteDigitsTable[EXTRACT_FIELD(action->type, AddressSize)] - 1; i >= 0; i--)
		{
			if(i == editCursor)
				stringsBuf += sprintf(stringsBuf, "[%X]", (displayValue & kIncrementMaskTable[i]) / kIncrementValueTable[i]);
			else
				stringsBuf += sprintf(stringsBuf, "%X", (displayValue & kIncrementMaskTable[i]) / kIncrementValueTable[i]);
		}
	}
	else
		stringsBuf += sprintf(stringsBuf, "%.*X", kSearchByteDigitsTable[EXTRACT_FIELD(action->type, AddressSize)], displayValue);

	if(TEST_FIELD(action->type, ValueSelectNegative))
		stringsBuf += sprintf(	stringsBuf, " <%.*X>",
								kSearchByteDigitsTable[EXTRACT_FIELD(action->type, AddressSize)],
								(~displayValue + 1) & kSearchByteMaskTable[EXTRACT_FIELD(action->type, AddressSize)]);

	if(!isBCD)
		stringsBuf += sprintf(stringsBuf, " (%d)", displayValue);

	stringsBuf += sprintf(stringsBuf, "\n\t OK ");

	/* print it */
	ui_draw_message_window(buf);

	/********** KEY HANDLING **********/
	if(ui_pressed_repeat_throttle(machine, IPT_UI_UP, kHorizontalFastKeyRepeatRate))
	{
		if(editActive)
		{
			if(displayValue == max)
				displayValue = min;
			else
			{
				displayValue = isBCD ? DoIncrementDecField(displayValue, editCursor) : DoIncrementHexField(displayValue, editCursor);

				if(displayValue > max)
					displayValue = max;
			}
		}
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_DOWN, kHorizontalFastKeyRepeatRate))
	{
		if(editActive)
		{
			if(displayValue == min)
				displayValue = max;

			else
			{
				displayValue = isBCD ? DoDecrementDecField(displayValue, editCursor) : DoDecrementHexField(displayValue, editCursor);

				if(displayValue > max)
					displayValue = max;
			}
		}
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_LEFT, kHorizontalFastKeyRepeatRate))
	{
		if(editActive)
		{
			if(++editCursor > kSearchByteDigitsTable[EXTRACT_FIELD(action->type, AddressSize)] - 1)
				editCursor = 0;
		}
		else
		{
			if(displayValue-- == min)
				displayValue = max;

			if(isBCD)
				displayValue = DecimalToBCD(BCDToDecimal(displayValue));
		}
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_RIGHT, kHorizontalFastKeyRepeatRate))
	{
		if(editActive)
		{
			if(--editCursor < 0)
				editCursor = kSearchByteDigitsTable[EXTRACT_FIELD(action->type, AddressSize)] - 1;
		}
		else
		{
			if(++displayValue > max)
				displayValue = min;

			if(isBCD)
				displayValue = DecimalToBCD(BCDToDecimal(displayValue));
		}
	}
	else if(input_ui_pressed(machine, IPT_UI_SELECT))
	{
		int i;

		if(editActive)
			editActive = 0;

		/* adjust a value */
		if(TEST_FIELD(action->type, ValueSelectMinimumDisplay))
			displayValue--;

		if(TEST_FIELD(action->type, ValueSelectMinimum) && !displayValue)
			displayValue = 1;

		if(TEST_FIELD(action->type, ValueSelectNegative) && displayValue)
			displayValue = (~displayValue + 1) & kSearchByteMaskTable[EXTRACT_FIELD(action->type, AddressSize)];

		/* copy value */
		for(i = 0; i < entry->actionListLength; i++)
		{
			CheatAction * destination = &entry->actionList[i];

			int copyValue = displayValue;

			if(	EXTRACT_FIELD(destination->type, CodeType) == kCodeType_VWrite ||
				EXTRACT_FIELD(destination->type, CodeType) == kCodeType_VRWrite)
			{
				destination->data = displayValue;
			}
			else if(EXTRACT_FIELD(destination->type, Link) == kLink_CopyLink)
			{
				int newData = destination->originalDataField;

				if(TEST_FIELD(destination->type, LinkValueSelectBCD))
				{
					copyValue	= BCDToDecimal(DecimalToBCD(copyValue));
					newData		= BCDToDecimal(DecimalToBCD(newData));
				}

				copyValue += newData;

				if(TEST_FIELD(destination->type, LinkValueSelectBCD))
					copyValue = DecimalToBCD(BCDToDecimal(copyValue));

				if(TEST_FIELD(destination->type, LinkValueSelectNegative))
					copyValue = (~copyValue + 1) & kSearchByteMaskTable[EXTRACT_FIELD(destination->type, AddressSize)];

				destination->data = copyValue;
			}
		}

		ActivateCheat(entry);

		menu->sel = -1;
	}
	else if(input_ui_pressed(machine, IPT_UI_EDIT_CHEAT))
	{
		editActive ^= 1;
		editCursor = 0;
	}
	else if(input_ui_pressed(machine, IPT_UI_CANCEL))
	{
		if(editActive)
			editActive = 0;
		else
			menu->sel = -1;
	}

	/* ##### EDIT ##### */
	if(!editActive)
	{
		UINT32 temp;

		temp			= displayValue;
		displayValue	= do_edit_hex_field(machine, displayValue) & kCheatSizeMaskTable[EXTRACT_FIELD(action->type, AddressSize)];

		if(displayValue != temp)
		{
			if(isBCD)
				displayValue = DecimalToBCD(BCDToDecimal(displayValue));

			if(displayValue < min)	displayValue = max;
			if(displayValue > max)	displayValue = min;
		}
	}

	return menu->sel + 1;
}

/*------------------------------------------------------------
  user_select_label_menu - management for label-select menu
------------------------------------------------------------*/

static int user_select_label_menu(running_machine *machine, cheat_menu_stack *menu)
{
	int				i;
	UINT8			total	= 0;
	const char		** menuItem;
	char			** buf;
	CheatEntry		* entry	= &cheatList[menu->pre_sel];

	/* required items = (total items + return + terminator) & (strings buf * total items [max chars = 100]) */
	request_strings(entry->actionListLength + 2, entry->actionListLength, 100, 0);
	menuItem	= menu_strings.main_list;
	buf			= menu_strings.main_strings;

	/* first setting : compute cursor position from previous menu */
	if(menu->first_time)
	{
		menu->sel = entry->flags & kCheatFlag_OneShot ? entry->selection - 1 : entry->selection;
		menu->first_time = 0;
	}

	/********** MENU CONSTRUCTION **********/
	for(i = 0; i < entry->labelIndexLength; i++)
	{
		if(!i)
		{
			if(!(entry->flags & kCheatFlag_OneShot))
			{
				/* add "OFF" item if not one shot at the first time */
				if(!entry->selection)
					sprintf(buf[total], "[ Off ]");
				else
					sprintf(buf[total], "Off");

				menuItem[total] = buf[total];
				total++;
			}

			continue;
		}
		else if(i == entry->selection)
			sprintf(buf[total], "[ %s ]", entry->actionList[entry->labelIndex[i]].optionalName);
		else
			sprintf(buf[total], "%s", entry->actionList[entry->labelIndex[i]].optionalName);

		menuItem[total] = buf[total];
		total++;
	}

	/* ##### RETURN ##### */
	menuItem[total++] = "Return to Prior Menu";

	/* ##### TERMINATE ARRAY ##### */
	menuItem[total] = NULL;

	/* adjust current cursor position */
	ADJUST_CURSOR(menu->sel, total)

	/* draw it */
	old_style_menu(menuItem, NULL, NULL, menu->sel, 0);

	/********** KEY HANDLING **********/
	if(ui_pressed_repeat_throttle(machine, IPT_UI_UP, kVerticalKeyRepeatRate))
	{
		CURSOR_TO_PREVIOUS(menu->sel, total)
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_DOWN, kVerticalKeyRepeatRate))
	{
		CURSOR_TO_NEXT(menu->sel, total)
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_PAGE_UP, kVerticalKeyRepeatRate))
	{
		CURSOR_PAGE_UP(menu->sel);
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_PAGE_DOWN, kVerticalKeyRepeatRate))
	{
		CURSOR_PAGE_DOWN(menu->sel, total)
	}
	else if(input_ui_pressed(machine, IPT_UI_SELECT))
	{
		if(menu->sel != total - 1)
		{
			entry->selection = entry->flags & kCheatFlag_OneShot ? menu->sel + 1 : menu->sel;

			/* set new label index */
			if(entry->selection)
				ActivateCheat(entry);
			else
				DeactivateCheat(entry);

			/* NOTE : the index number of master code should be stored into 1st table */
			if(TEST_FIELD(entry->actionList[entry->labelIndex[0]].type, LabelSelectQuickClose))
				menu->sel = -1;
		}
		else
			menu->sel = -1;
	}
	else if(input_ui_pressed(machine, IPT_UI_CANCEL))
		menu->sel = -1;

	return menu->sel + 1;
}

/*------------------------------------------------
  comment_menu - management simple comment menu
------------------------------------------------*/

static int comment_menu(running_machine *machine, cheat_menu_stack *menu)
{
	char		buf[2048];
	const char	* comment;
	CheatEntry	* entry = &cheatList[menu->pre_sel];

	/* first setting : NONE */
	if(menu->first_time)
		menu->first_time = 0;

	/********** MENU CONSTRUCTION **********/
	if(entry->comment && entry->comment[0])
		comment = entry->comment;
	else
		comment = "this code doesn't have a comment";

	sprintf(buf, "%s\n\t OK ", comment);

	/* print it */
	ui_draw_message_window(buf);

	/********** KEY HANDLING **********/
	if(input_ui_pressed(machine, IPT_UI_SELECT) || input_ui_pressed(machine, IPT_UI_CANCEL))
		menu->sel = -1;

	return menu->sel + 1;
}

/*-----------------------------------------------------------
  extend_comment_menu - management multi-line comment menu
-----------------------------------------------------------*/

static int extend_comment_menu(running_machine *machine, cheat_menu_stack *menu)
{
	int			i;
	UINT8		total	= 0;
	const char	** menuItem;
	char		** buf;
	CheatEntry	* entry	= &cheatList[menu->pre_sel];

	if(!entry->actionListLength)
		return 0;

	/* first setting : NONE */
	if(menu->first_time)
		menu->first_time = 0;

	/* required items = (display comment lines + return + terminator) & (strings buf * display comment lines [max char = 100]) */
	request_strings(entry->actionListLength + 1, entry->actionListLength - 1, 100, 0);
	menuItem 	= menu_strings.main_list;
	buf			= menu_strings.main_strings;

	/********** MENU CONSTRUCTION **********/
	for(i = 1; i < entry->actionListLength; i++)
	{
		CheatAction * action = &entry->actionList[i];

		/* NOTE : don't display the comment in master code */
		if(i)
		{
			sprintf(buf[total], "%s", action->optionalName);
			menuItem[total] = buf[total];
			total++;
		}
	}

	/* ##### OK ##### */
	menuItem[total++] = "OK";

	/* ##### TERMINATE ARRAY ##### */
	menuItem[total] = NULL;

	/* adjust cursor position */
	ADJUST_CURSOR(menu->sel, total)

	/* draw it */
	old_style_menu(menuItem, NULL, NULL, menu->sel, 0);

	/********** KEY HANDLING **********/
	if(ui_pressed_repeat_throttle(machine, IPT_UI_UP, kVerticalKeyRepeatRate))
	{
		CURSOR_TO_PREVIOUS(menu->sel, total)
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_DOWN, kVerticalKeyRepeatRate))
	{
		CURSOR_TO_NEXT(menu->sel, total)
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_PAGE_UP, kVerticalKeyRepeatRate))
	{
		CURSOR_PAGE_UP(menu->sel)
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_PAGE_DOWN, kVerticalKeyRepeatRate))
	{
		CURSOR_PAGE_DOWN(menu->sel, total)
	}
	else if(input_ui_pressed(machine, IPT_UI_SELECT) || input_ui_pressed(machine, IPT_UI_CANCEL))
		menu->sel = -1;

	return menu->sel + 1;
}

/*------------------------------------------------------
  cheat_main_menu - management for cheat general menu
------------------------------------------------------*/

static int cheat_main_menu(running_machine *machine, cheat_menu_stack *menu)
{
	enum
	{
		kMenu_EnableDisable = 0,
		kMenu_AddEdit,
		kMenu_Search,
		kMenu_ViewResult,
		kMenu_ChooseWatch,
		kMenu_Options,
#ifdef MAME_DEBUG
		kMenu_Debug,
#endif
		kMenu_Return,
		kMenu_Max
	};

	UINT8			total		= 0;
	ui_menu_item	menu_item[kMenu_Max + 1];

	/* allocate memory for item list */
	memset(menu_item, 0, sizeof(menu_item));

	/* first setting : NONE */
	if(menu->first_time)
		menu->first_time = 0;

	/********** MENU CONSTRUCION **********/
	/* ##### Enable/Disable a Cheat ##### */
	menu_item[total++].text = "Enable/Disable a Cheat";

	/* ##### Add/Edit a Cheat ##### */
	menu_item[total++].text = "Add/Edit a Cheat";

	/* ##### Search a Cheat ##### */
	switch(EXTRACT_FIELD(cheatOptions, SearchBox))
	{
		case kSearchBox_Minimum:
			menu_item[total++].text = "Search a Cheat (Minimum Mode)";
			break;

		case kSearchBox_Standard:
			menu_item[total++].text = "Search a Cheat (Standard Mode)";
			break;

		case kSearchBox_Advanced:
			menu_item[total++].text = "Search a Cheat (Advanced Mode)";
			break;

		default:
			menu_item[total++].text = "Unknown Search Menu";
	}

	/* ##### View Last Result ##### */
	menu_item[total++].text = "View Last Results";

	/* ##### Configure Watchpoints ##### */
	menu_item[total++].text = "Configure Watchpoints";

	/* ##### Options ##### */
	menu_item[total++].text = "Options";

#ifdef MAME_DEBUG
	/* ##### Debug  ##### */
	menu_item[total++].text = "Debug Viewer";
#endif

	/* ##### Return to the MAME general menu ##### */
	menu_item[total++].text = "Return to Main Menu";

	/* ##### Terminate Array ##### */
	menu_item[total].text = NULL;

	ADJUST_CURSOR(menu->sel, total)

	/* print it */
	ui_menu_draw(menu_item, total, menu->sel, NULL);

	/********** KEY HANDLING **********/
	if(ui_pressed_repeat_throttle(machine, IPT_UI_UP, kVerticalKeyRepeatRate))
	{
		CURSOR_TO_PREVIOUS(menu->sel, total)
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_DOWN, kVerticalKeyRepeatRate))
	{
		CURSOR_TO_NEXT(menu->sel, total)
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_PAGE_UP, kVerticalKeyRepeatRate))
	{
		CURSOR_PAGE_UP(menu->sel)
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_PAGE_DOWN, kVerticalKeyRepeatRate))
	{
		CURSOR_PAGE_DOWN(menu->sel, total)
	}
	else if(input_ui_pressed(machine, IPT_UI_SELECT))
	{
		switch(menu->sel)
		{
			case kMenu_EnableDisable:
				cheat_menu_stack_push(enable_disable_cheat_menu, cheat_main_menu, menu->sel);
				break;

			case kMenu_AddEdit:
				cheat_menu_stack_push(add_edit_cheat_menu, cheat_main_menu, 0);
				break;

			case kMenu_Search:
				switch(EXTRACT_FIELD(cheatOptions, SearchBox))
				{
					case kSearchBox_Minimum:
						cheat_menu_stack_push(search_minimum_menu, menu->handler, menu->sel);
						break;

					case kSearchBox_Standard:
						cheat_menu_stack_push(search_standard_menu, menu->handler, menu->sel);
						break;

					case kSearchBox_Advanced:
						cheat_menu_stack_push(search_advanced_menu, menu->handler, menu->sel);
				}
				break;

			case kMenu_ViewResult:
				cheat_menu_stack_push(view_search_result_menu, menu->handler, menu->sel);
				break;

			case kMenu_ChooseWatch:
				cheat_menu_stack_push(choose_watch_menu, menu->handler, menu->sel);
				break;

			case kMenu_Options:
				cheat_menu_stack_push(select_option_menu, menu->handler, menu->sel);
				break;

#ifdef MAME_DEBUG
			case kMenu_Debug:
				cheat_menu_stack_push(debug_cheat_menu, menu->handler, menu->sel);
				break;
#endif
			case kMenu_Return:
				menu->sel = -1;
		}
	}
	else if(input_ui_pressed(machine, IPT_UI_RELOAD_CHEAT))
	{
		ReloadCheatDatabase(machine);
	}
	else if(input_ui_pressed(machine, IPT_UI_CANCEL))
	{
		menu->sel = -1;
	}

	return menu->sel + 1;
}

/*-----------------------------------------------------------------
  enable_disable_cheat_menu - management for Enable/Disable menu
-----------------------------------------------------------------*/

static int enable_disable_cheat_menu(running_machine *machine, cheat_menu_stack *menu)
{
	int				i;
	UINT8			total			= 0;
	UINT8			length			= cheatListLength ? cheatListLength : 2;
	UINT8			isEmpty			= 0;
	UINT8 *			menuIndex;
	static INT8		currentLayer;
	const char		** menu_item;
	const char		** menu_subitem;
	char			* flagBuf;
	CheatEntry		* entry;

	/* first setting : always starts with root layer level */
	if(menu->first_time)
	{
		currentLayer = 0;
		menu->first_time = 0;
	}

	/* required items = total codes + return + terminator */
	request_strings(length + 2, 0, 0, 0);

	menu_item		= menu_strings.main_list;
	menu_subitem	= menu_strings.sub_list;
	flagBuf			= menu_strings.flag_list;

	menuIndex = malloc_or_die(sizeof(INT8) * (length + 1));
	memset(menuIndex, 0, sizeof(INT8) * (length + 1));

	/********** MENU CONSTRUCTION **********/
	for(i = 0; i < cheatListLength; i++)
	{
		CheatEntry * traverse	= &cheatList[i];

		if(total >= fullMenuPageHeight) break;

		/* when return to previous layer, set cursor position to previous layer label code */
		if(traverse->flags & kCheatFlag_LayerSelected)
		{
			menu->sel = total;
			traverse->flags &= ~kCheatFlag_LayerSelected;
		}

		if(traverse->layerIndex == currentLayer)
		{
			/* ##### SEPARAOTR ##### */
			if(traverse->flags & kCheatFlag_Separator)
			{
				menu_item[total] = MENU_SEPARATOR_ITEM;
				menu_subitem[total] = NULL;
				menuIndex[total++] = i;
			}
			else
			{
				/* ##### NAME ##### */
				if(traverse->name)
					menu_item[total] = traverse->name;
				else
					menu_item[total] = "null name";

				/* ##### SUB ITEM ##### */
				if(traverse->flags & kCheatFlag_HasWrongCode)
				{
					/* "LOCKED" if wrong code */
					menu_subitem[total] = "Locked";
					traverse->selection = 0;
				}
				else if(traverse->flags & kCheatFlag_Select)
				{
					/* label or "OFF" if label-selection though one-shot should ignore "OFF" */
					if(traverse->selection)
						menu_subitem[total] = traverse->actionList[traverse->labelIndex[traverse->selection]].optionalName;
					else
						menu_subitem[total] = "Off";
				}
				else if(traverse->flags & kCheatFlag_ExtendComment)
				{
					/* "READ" if extend comment code */
					menu_subitem[total] = "Read";
				}
				else if(traverse->flags & kCheatFlag_LayerIndex)
				{
					/* ">>>" (next layer) if layer label code */
					menu_subitem[total] = ">>>";
				}
				else if(!(traverse->flags & kCheatFlag_Null))
				{
					/* otherwise, add sub-items */
					if(traverse->flags & kCheatFlag_OneShot)
						menu_subitem[total] = "Set";
					else if(traverse->flags & kCheatFlag_Active)
						menu_subitem[total] = "On";
					else
						menu_subitem[total] = "Off";
				}

				/* set highlight flag if comment is set */
				if(traverse->comment && traverse->comment[0])
					flagBuf[total] = 1;
				else
					flagBuf[total] = 0;
				menuIndex[total++] = i;
			}
		}
		else if(traverse->flags & kCheatFlag_LayerIndex)
		{
			if(currentLayer == traverse->actionList[0].data)
			{
				/* specified handling previous layer label code with "<<<" (previous layer) */
				if(traverse->comment)
					menu_item[total] = traverse->comment;
				else
					menu_item[total] = "Return to Prior Layer";
				menu_subitem[total] = "<<<";
				flagBuf[total] = 0;
				menuIndex[total++] = i;
			}
		}
	}

	/* if no code, set special message */
	if(!cheatListLength)
	{
		if(foundCheatDatabase)
		{
			/* the database is found but no code */
			menu_item[total]	= "there are no cheats for this game";
			menu_subitem[total]	= NULL;
			menuIndex[total]	= total;
			flagBuf[total++]	= 0;
			isEmpty				= 1;
		}
		else
		{
			/* the database itself is not found */
			menu_item[total]	= "cheat database not found";
			menu_subitem[total]	= NULL;
			menuIndex[total]	= total;
			flagBuf[total++]	= 0;

			menu_item[total]	= "unzip it and place it in the MAME directory";
			menu_subitem[total]	= NULL;
			menuIndex[total]	= total;
			flagBuf[total++]	= 0;
			isEmpty				= 2;
		}
	}
	else if(currentLayer && total == 1)
	{
		/* selected layer doesn't have code */
		menu_item[total]	= "selected layer doesn't have sub code";
		menu_subitem[total]	= NULL;
		menuIndex[total]	= menuIndex[total - 1];
		flagBuf[total++]	= 0;
		isEmpty = 3;
	}

	/* ##### RETURN ##### */
	if(currentLayer)
		menu_item[total] = "Return to Root Layer";
	else
		menu_item[total] = "Return to Prior Menu";
	menu_subitem[total] = NULL;
	flagBuf[total++] = 0;

	/* ##### TERMINATE ARRAY ##### */
	menu_item[total] = NULL;
	menu_subitem[total] = NULL;
	flagBuf[total] = 0;

	/* adjust cursor position */
	switch(isEmpty)
	{
		default:
			ADJUST_CURSOR(menu->sel, total)

			/* if cursor is on comment code, skip it */
			while(menu->sel < total - 1 && (cheatList[menuIndex[menu->sel]].flags & kCheatFlag_Null))
				menu->sel++;
			break;

		case 1:
		case 2:
			/* no database or code, unselectable message line */
			menu->sel = total - 1;
			break;

		case 3:
			/* no code in sub layer, unselectable message line */
			if(menu->sel == 1)
				menu->sel = 0;
	}

	/* draw it */
	old_style_menu(menu_item, menu_subitem, flagBuf, menu->sel, 3);

	/********** KEY HANDLING **********/
	if(ui_pressed_repeat_throttle(machine, IPT_UI_UP, kVerticalKeyRepeatRate))
	{
		switch(isEmpty)
		{
			default:
				CURSOR_TO_PREVIOUS(menu->sel, total)
				if(cheatListLength)
				{
					/* if cursor is on comment code, skip it */
					for(i = 0; (i < fullMenuPageHeight / 2) && menu->sel != total - 1 && (cheatList[menuIndex[menu->sel]].flags & kCheatFlag_Null); i++)
					{
						if(--menu->sel < 0)
							menu->sel = total - 1;
					}
				}
				break;

			case 1:
				break;

			case 2:
				if(menu->sel)
					menu->sel = 0;
				else
					menu->sel = 2;
		}
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_DOWN, kVerticalKeyRepeatRate))
	{
		switch(isEmpty)
		{
			default:
				CURSOR_TO_NEXT(menu->sel, total)
				if(cheatListLength)
				{
					/* if cursor is on comment code, skip it */
					for(i = 0; (i < fullMenuPageHeight / 2) && menu->sel < total - 1 && (cheatList[menuIndex[menu->sel]].flags & kCheatFlag_Null); i++)
						menu->sel++;
				}

			case 1:
				break;

			case 2:
				if(menu->sel)
					menu->sel = 0;
				else
					menu->sel = 2;
		}
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_PAGE_UP, kVerticalKeyRepeatRate))
	{
		CURSOR_PAGE_UP(menu->sel)
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_PAGE_DOWN, kVerticalKeyRepeatRate))
	{
		CURSOR_PAGE_DOWN(menu->sel, total)
	}

	if(menu->sel >= 0 && menu->sel < total - 1)
		entry = &cheatList[menuIndex[menu->sel]];
	else
		entry = NULL;

	if(entry && !(entry->flags & kCheatFlag_Null) && !(entry->flags & kCheatFlag_ExtendComment) && !(entry->flags & kCheatFlag_HasWrongCode))
	{
		UINT8 toggle = 0;

		if(ui_pressed_repeat_throttle(machine, IPT_UI_LEFT, kHorizontalSlowKeyRepeatRate))
		{
			if(entry->flags & kCheatFlag_Select)
			{
				if(TEST_FIELD(entry->actionList[entry->labelIndex[0]].type, LabelSelectUseSelector))
				{
					/* activate label selector */
					cheat_menu_stack_push(user_select_label_menu, menu->handler, menu->sel);
				}
				else
				{
					/* select previous label */
					if(--entry->selection < 0)
						entry->selection = entry->labelIndexLength - 1;

					/* NOTE : one shot cheat should not be activated by changing label */
					if(!entry->labelIndex[entry->selection])
						DeactivateCheat(entry);
					else if(!(entry->flags & kCheatFlag_OneShot) && !(entry->flags & kCheatFlag_Active))
						ActivateCheat(entry);
				}
			}
			else if(entry->flags & kCheatFlag_LayerIndex)
			{
				/* change layer level */
				toggle = 2;
			}
			else if(!(entry->flags & kCheatFlag_OneShot))
			{
				/* toggle ON/OFF */
				toggle = 1;
			}
		}
		else if(ui_pressed_repeat_throttle(machine, IPT_UI_RIGHT, kHorizontalSlowKeyRepeatRate))
		{
			if(entry->flags & kCheatFlag_Select)
			{
				/* label-selection special handling */
				if(TEST_FIELD(entry->actionList[entry->labelIndex[0]].type, LabelSelectUseSelector))
				{
					/* activate label selector */
					cheat_menu_stack_push(user_select_label_menu, menu->handler, menu->sel);
				}
				else
				{
					/* select next label index */
					if(++entry->selection >= entry->labelIndexLength)
						entry->selection = 0;

					/* NOTE : one shot cheat should not be activated by changing label */
					if(!entry->labelIndex[entry->selection])
						DeactivateCheat(entry);
					else if(!(entry->flags & kCheatFlag_OneShot) && !(entry->flags & kCheatFlag_Active))
						ActivateCheat(entry);
				}
			}
			else if(entry->flags & kCheatFlag_LayerIndex)
			{
				/* change layer level */
				toggle = 2;
			}
			else if(!(entry->flags & kCheatFlag_OneShot))
			{
				/* toggle ON/OFF */
				toggle = 1;
			}
		}

		switch(toggle)
		{
			case 1:
				{
					int active = (entry->flags & kCheatFlag_Active) ^ 1;

					if((entry->flags & kCheatFlag_UserSelect) && active)
					{
						/* activate value-selection menu */
						cheat_menu_stack_push(user_select_value_menu, menu->handler, menu->sel);
					}
					else
					{
						if(active)
							ActivateCheat(entry);
						else
							DeactivateCheat(entry);
					}
				}
				break;

			case 2:
				{
					/* go to next/previous layer level */
					if(entry->actionList[0].data == currentLayer)
						currentLayer = entry->actionList[0].address;
					else
						currentLayer = entry->actionList[0].data;

					entry->flags |= kCheatFlag_LayerSelected;
				}
				break;

			default:
				break;
		}
	}

	if(input_ui_pressed(machine, IPT_UI_SELECT))
	{
		if(!entry)
		{
			if(currentLayer)
				currentLayer = menu->sel = 0;
			else
				menu->sel = -1;
		}
		else if(!(entry->flags & kCheatFlag_Null))
		{
			if(ShiftKeyPressed() && (entry->comment && entry->comment[0]))
			{
				/* display comment */
				cheat_menu_stack_push(comment_menu, menu->handler, menu->sel);
			}
			else if(entry->flags & kCheatFlag_ExtendComment)
			{
				/* activate extend comment menu */
				cheat_menu_stack_push(extend_comment_menu, menu->handler, menu->sel);
			}
			else if(entry->flags & kCheatFlag_UserSelect)
			{
				/* activate value-selection menu */
				cheat_menu_stack_push(user_select_value_menu, menu->handler, menu->sel);
			}
			else if((entry->flags & kCheatFlag_Select) && TEST_FIELD(entry->actionList[entry->labelIndex[0]].type, LabelSelectUseSelector))
			{
				/* activate label selector */
				cheat_menu_stack_push(user_select_label_menu, menu->handler, menu->sel);
			}
			else if(entry->flags & kCheatFlag_LayerIndex)
			{
				/* go to next/previous layer level */
				if(entry->actionList[0].data == currentLayer)
					currentLayer = entry->actionList[0].address;
				else
					currentLayer = entry->actionList[0].data;

				entry->flags |= kCheatFlag_LayerSelected;
			}
			else if(entry->flags & kCheatFlag_HasWrongCode)
			{
				/* analyse wrong format code */
				cheat_menu_stack_push(analyse_cheat_menu, menu->handler, menu->sel);
			}
			else
			{
				/* activate selected code */
				ActivateCheat(entry);
			}
		}
	}
	else if(input_ui_pressed(machine, IPT_UI_SAVE_CHEAT))
	{
		if(ShiftKeyPressed())
		{
			/* shift + save : save all codes */
			for(i = 0; i < cheatListLength; i++)
				SaveCheatCode(machine, &cheatList[i]);

			messageTimer = DEFAULT_MESSAGE_TIME;
			messageType = kCheatMessage_AllCheatSaved;
		}
		else if(ControlKeyPressed())
		{
			/* ctrl + save : save activation key */
			SaveActivationKey(machine, entry, menuIndex[menu->sel]);
		}
		else if(AltKeyPressed())
		{
			/* alt + save : save pre-enable */
			SavePreEnable(machine, entry, menuIndex[menu->sel]);
		}
	}
	else if(input_ui_pressed(machine, IPT_UI_ADD_CHEAT))
	{
		AddCheatBefore(menuIndex[menu->sel]);
	}
	else if(input_ui_pressed(machine, IPT_UI_DELETE_CHEAT))
	{
		if(entry)
			DeleteCheatAt(menuIndex[menu->sel]);
		else
		{
			messageType		= kCheatMessage_FailedToDelete;
			messageTimer	= DEFAULT_MESSAGE_TIME;
		}
	}
	else if(input_ui_pressed(machine, IPT_UI_EDIT_CHEAT))
	{
		if(entry)
		{
			cheat_menu_stack_push(command_add_edit_menu, menu->handler, menu->sel);
		}
	}
	else if(input_ui_pressed(machine, IPT_UI_WATCH_VALUE))
	{
		WatchCheatEntry(entry, 0);
	}
	else if(input_ui_pressed(machine, IPT_UI_RELOAD_CHEAT))
	{
		ReloadCheatDatabase(machine);
	}
	else if(input_ui_pressed(machine, IPT_UI_CANCEL))
	{
		/* NOTE : cancel button return cheat main menu directly */
		menu->sel = -1;
	}

	free(menuIndex);

	return menu->sel + 1;
}

/*-----------------------------------------------------------
  add_edit_cheat_menu - management for Add/Edit cheat menu
-----------------------------------------------------------*/

static int add_edit_cheat_menu(running_machine *machine, cheat_menu_stack *menu)
{
	int				i;
	UINT8			total			= 0;
	const char		** menu_item;
	CheatEntry		* entry;

	/* first setting : NONE */
	if(menu->first_time)
		menu->first_time = 0;

	/* required items = total codes + return + terminator */
	request_strings(cheatListLength + 2, 0, 0, 0);
	menu_item = menu_strings.main_list;

	/********** MENU CONSTRUCTION **********/
	for(i = 0; i < cheatListLength; i++)
	{
		CheatEntry * traverse = &cheatList[i];

		/* ##### NAME ##### */
		if(traverse->name)
			menu_item[total++] = traverse->name;
		else
			menu_item[total++] = "(none)";
	}

	/* ##### RETURN ##### */
	menu_item[total++] = "Return to Prior Menu";

	/* ##### TERMINATE ARRAY ##### */
	menu_item[total] = NULL;

	/* adjust cursor position */
	ADJUST_CURSOR(menu->sel, total)

	/* draw it */
	old_style_menu(menu_item, NULL, NULL, menu->sel, 0);

	if(menu->sel < total - 1)
		entry = &cheatList[menu->sel];
	else
		entry = NULL;

	/********** KEY HANDLING **********/
	if(ui_pressed_repeat_throttle(machine, IPT_UI_UP, kVerticalKeyRepeatRate))
	{
		CURSOR_TO_PREVIOUS(menu->sel, total)
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_DOWN, kVerticalKeyRepeatRate))
	{
		CURSOR_TO_NEXT(menu->sel, total)
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_PAGE_UP, kVerticalKeyRepeatRate))
	{
		CURSOR_PAGE_UP(menu->sel)
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_PAGE_DOWN, kVerticalKeyRepeatRate))
	{
		CURSOR_PAGE_DOWN(menu->sel, total)
	}
	else if(input_ui_pressed(machine, IPT_UI_SELECT))
	{
		if(entry)
		{
			if(entry->flags & kCheatFlag_HasWrongCode)
			{
				cheat_menu_stack_push(analyse_cheat_menu, menu->handler, menu->sel);
			}
			else
				cheat_menu_stack_push(command_add_edit_menu, menu->handler, menu->sel);
		}
		else
			menu->sel = -1;
	}
	else if(input_ui_pressed(machine, IPT_UI_RELOAD_CHEAT))
	{
		ReloadCheatDatabase(machine);
	}
	else if(input_ui_pressed(machine, IPT_UI_EDIT_CHEAT))
	{
		if(entry)
		{
			if(entry->flags & kCheatFlag_HasWrongCode)
			{
				cheat_menu_stack_push(analyse_cheat_menu, menu->handler, menu->sel);
			}
			else
				cheat_menu_stack_push(command_add_edit_menu, menu->handler, menu->sel);
		}
	}
	else if(input_ui_pressed(machine, IPT_UI_ADD_CHEAT))
	{
		AddCheatBefore(menu->sel);
	}
	else if(input_ui_pressed(machine, IPT_UI_DELETE_CHEAT))
	{
		DeleteCheatAt(menu->sel);
	}
	else if(input_ui_pressed(machine, IPT_UI_SAVE_CHEAT))
	{
		if(ShiftKeyPressed())
		{
			/* shift + save = save all codes */
			for(i = 0; i < cheatListLength; i++)
				SaveCheatCode(machine, &cheatList[i]);

			messageTimer	= DEFAULT_MESSAGE_TIME;
			messageType		= kCheatMessage_AllCheatSaved;
		}
		else if(ControlKeyPressed())
		{
			if((entry->flags & kCheatFlag_HasActivationKey1) || (entry->flags & kCheatFlag_HasActivationKey2))
				/* ctrl + save = save activation key */
				SaveActivationKey(machine, entry, menu->sel);
		}
		else if(AltKeyPressed())
		{
			/* alt + save = save pre-enable code */
			SavePreEnable(machine, entry, menu->sel);
		}
		else
		{
			SaveCheatCode(machine, entry);
		}
	}
	else if(input_ui_pressed(machine, IPT_UI_WATCH_VALUE))
	{
		WatchCheatEntry(entry, 0);
	}
	else if(input_ui_pressed(machine, IPT_UI_CANCEL))
	{
		menu->sel = -1;
	}

	return menu->sel + 1;
}

/*-----------------------------------------------------------
  command_add_edit_menu - management for code command menu
-----------------------------------------------------------*/

static int command_add_edit_menu(running_machine *machine, cheat_menu_stack *menu)
{
	enum{
		kMenu_EditCheat = 0,
		kMenu_ReloadDatabase,
		kMenu_WatchCheat,
		kMenu_SaveCheat,
		kMenu_SaveActivationKey,
		kMenu_SavePreEnable,
		kMenu_SaveAllCodes,
		kMenu_AddCode,
		kMenu_DeleteCode,
#ifdef MAME_DEBUG
		kMenu_ConvertFormat,
		kMenu_AnalyseCode,
#endif
		kMenu_Return,
		kMenu_Max };

	int				i;
	UINT8			total = 0;
	ui_menu_item	menu_item[kMenu_Max + 1];
	CheatEntry		*entry = &cheatList[menu->pre_sel];

	memset(menu_item, 0, sizeof(menu_item));

	/* first setting : NONE */
	if(menu->first_time)
		menu->first_time = 0;

	/********** MENU CONSTRUCION **********/
	if(entry->flags & kCheatFlag_OldFormat)
		menu_item[total++].text = "Edit Code";
	else
		menu_item[total++].text = "View Code";
	menu_item[total++].text = "Reload Database";
	menu_item[total++].text = "Watch Code";
	menu_item[total++].text = "Save Code";
	menu_item[total++].text = "Save Activation Key";
	menu_item[total++].text = "Save PreEnable";
	menu_item[total++].text = "Save All Codes";
	menu_item[total++].text = "Add New Code";
	menu_item[total++].text = "Delete Code";
#ifdef MAME_DEBUG
	if(entry->flags & kCheatFlag_OldFormat)
		menu_item[total++].text = "Convert To New Format";
	else
		menu_item[total++].text = "Convert To Old Format";
	menu_item[total++].text = "Analyse Code";
#endif
	menu_item[total++].text = "Return to Prior Menu";
	menu_item[total].text = NULL;

	/* adjust cursor position */
	ADJUST_CURSOR(menu->sel, total)

	/* print it */
	ui_menu_draw(menu_item, total, menu->sel, NULL);

	/********** KEY HANDLING **********/
	if(ui_pressed_repeat_throttle(machine, IPT_UI_UP, kVerticalKeyRepeatRate))
	{
		CURSOR_TO_PREVIOUS(menu->sel, total)
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_DOWN, kVerticalKeyRepeatRate))
	{
		CURSOR_TO_NEXT(menu->sel, total)
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_PAGE_UP, kVerticalKeyRepeatRate))
	{
		CURSOR_PAGE_UP(menu->sel)
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_PAGE_DOWN, kVerticalKeyRepeatRate))
	{
		CURSOR_PAGE_DOWN(menu->sel, total)
	}
	else if(input_ui_pressed(machine, IPT_UI_SELECT))
	{
		switch(menu->sel)
		{
			case kMenu_EditCheat:
				if(entry->flags & kCheatFlag_OldFormat)
					cheat_menu_stack_push(edit_cheat_menu, menu->return_handler, menu->pre_sel);
				else
					cheat_menu_stack_push(view_cheat_menu, menu->return_handler, menu->pre_sel);
				break;

			case kMenu_ReloadDatabase:
				ReloadCheatDatabase(machine);
				cheat_menu_stack_pop();
				break;

			case kMenu_WatchCheat:
				WatchCheatEntry(entry, 0);
				break;

			case kMenu_SaveCheat:
				SaveCheatCode(machine, entry);
				break;

			case kMenu_SaveActivationKey:
				SaveActivationKey(machine, entry, menu->pre_sel);
				break;

			case kMenu_SavePreEnable:
				SavePreEnable(machine, entry, menu->pre_sel);
				break;

			case kMenu_SaveAllCodes:
				{
					for(i = 0; i < cheatListLength; i++)
						SaveCheatCode(machine, &cheatList[i]);

					messageTimer	= DEFAULT_MESSAGE_TIME;
					messageType		= kCheatMessage_AllCheatSaved;
				}
				break;

			case kMenu_AddCode:
				AddCheatBefore(menu->pre_sel);
				menu->sel = -1;
				break;

			case kMenu_DeleteCode:
				DeleteCheatAt(menu->pre_sel);
				menu->sel = -1;
				break;

#ifdef MAME_DEBUG
			case kMenu_ConvertFormat:
				if(!(entry->flags & kCheatFlag_OldFormat))
				{
					for(i = 0; i < entry->actionListLength; i++)
					{
						entry->actionList[i].flags |= kActionFlag_OldFormat;
						UpdateCheatInfo(entry, 0);
					}
				}
				else
				{
					for(i = 0; i < entry->actionListLength; i++)
					{
						entry->actionList[i].flags &= ~kActionFlag_OldFormat;
						UpdateCheatInfo(entry, 0);
					}
				}
				break;

			case kMenu_AnalyseCode:
				cheat_menu_stack_push(analyse_cheat_menu, menu->return_handler, menu->pre_sel);
				break;
#endif
			case kMenu_Return:
				menu->sel = -1;
				break;
		}
	}
	else if(input_ui_pressed(machine, IPT_UI_RELOAD_CHEAT))
	{
		ReloadCheatDatabase(machine);
	}
	else if(input_ui_pressed(machine, IPT_UI_CANCEL) || input_ui_pressed(machine, IPT_UI_LEFT) || input_ui_pressed(machine, IPT_UI_RIGHT))
	{
		menu->sel = -1;
	}

	return menu->sel + 1;
}

/*--------------------------------------------------
  edit_cheat_menu - management for edit code menu
--------------------------------------------------*/

static int edit_cheat_menu(running_machine *machine, cheat_menu_stack *menu)
{
#if 0
	static const char *const kTypeNames[] =
	{
		"Normal/Delay",
		"Wait",
		"Ignore Decrement",
		"Watch",
		"Comment",
		"Select"
	};

	static const char *const kOperationNames[] =
	{
		"Write",
		"Add/Subtract",
		"Force Range",
		"Set/Clear Bits",
		"Unused (4)",
		"Unused (5)",
		"Unused (6)",
		"Null"
	};

	static const char *const kAddSubtractNames[] =
	{
		"Add",
		"Subtract"
	};

	static const char *const kSetClearNames[] =
	{
		"Set",
		"Clear"
	};

	static const char *const kPrefillNames[] =
	{
		"None",
		"FF",
		"00",
		"01"
	};

	static const char *const kEndiannessNames[] =
	{
		"Normal",
		"Swap"
	};

	static const char *const kLocationNames[] =
	{
		"Normal",
		"Region",
		"Mapped Memory",
		"Custom",
		"Relative Address",
		"Unused (5)", "Unused (6)", "Unused (7)"
	};

	static const char *const kCustomLocationNames[] =
	{
		"Comment",
		"EEPROM",
		"Select",
		"Unused (3)",	"Unused (4)",	"Unused (5)",	"Unused (6)",	"Unused (7)",
		"Unused (8)",	"Unused (9)",	"Unused (10)",	"Unused (11)",	"Unused (12)",
		"Unused (13)",	"Unused (14)",	"Unused (15)",	"Unused (16)",	"Unused (17)",
		"Unused (18)",	"Unused (19)",	"Unused (20)",	"Unused (21)",	"Unused (22)",
		"Unused (23)",	"Unused (24)",	"Unused (25)",	"Unused (26)",	"Unused (27)",
		"Unused (28)",	"Unused (29)",	"Unused (30)",	"Unused (31)"
	};

	enum
	{
		kType_Name = 0,					//  text        name
										//  NOTE:   read from base cheat (for idx == 0)
		kType_ExtendName,				//  text        extraName
										//  NOTE:   read from subcheat for (idx > 0) && (cheat[0].type == Select)
		kType_Comment,					//  text        comment
										//  NOTE:   read from base cheat (for idx == 0)
		kType_ActivationKey1,			//  key         1st activationKey
										//  NOTE:   read from base cheat (for idx == 0)
		kType_ActivationKey2,			//  key         2nd activationKey
										//  NOTE:   read from base cheat (for idx == 0)
		kType_Code,						//  value       Type Field
										//  NOTE:   it's debug display so that must not modify directly
		kType_Link,						//  select      Link                  Off - On
		kType_LinkExtension,			//  select      Link Extension        Off - On
		kType_Type,						//  select      Type                  Normal/Delay - Watch - Comment - (label) Select
		kType_OneShot,					//  select      OneShot               Off - On
		kType_RestorePreviousValue,		//  select      RestorePreviousValue  Off - On
			// if(Type != Ignore Decrement)
			kType_Delay,				//  value       TypeParameter         0 - 7
			// else
			kType_IgnoreDecrementBy,	//  value       TypeParameter         0 - 7
			// if(Type == Watch)
			kType_WatchSize,			//  value       Data                  0x01 - 0xFF (stored as 0x00 - 0xFE)
										//  NOTE: value is packed in to 0x000000FF
			kType_WatchSkip,			//  value       Data                  0x00 - 0xFF
										//  NOTE: value is packed in to 0x0000FF00
			kType_WatchPerLine,			//  value       Data                  0x00 - 0xFF
										//  NOTE: value is packed in to 0x00FF0000
			kType_WatchAddValue,		//  value       Data                  -0x80 - 0x7F
										//  NOTE: value is packed in to 0xFF000000
			kType_WatchFormat,			//  select      TypeParameter         Hex - Decimal - Binary - ASCII
										//  NOTE: value is packed in to 0x03
			kType_WatchLabel,			//  select      TypeParameter         Off - On
										//  NOTE: value is packed in to 0x04
		kType_Operation,				//  select      Operation             Write - Add/Subtract - Force Range - Set/Clear Bits
		kType_OperationExtend,			//  select      Operation Extend      Off - On
										//  NOTE: Operation Extend is unused right now
			// if(LocationType != Relative Address && Operation == Write)
			kType_WriteMask,			//  value       extendData            0x00000000 - 0xFFFFFFFF
			// if(LocationType != Relative Address && Operation == Add/Subtract)
			kType_AddSubtract,			//  select      OperationParameter    Add - Subtract
				// if(OperationParameter == Add)
				kType_AddMaximum,		//  value       extendData            0x00000000 - 0xFFFFFFFF
				// else
				kType_SubtractMinimum,	//  value       extendData            0x00000000 - 0xFFFFFFFF
			// if(LocationType != Relative Address && Operation == Force Range)
			kType_RangeMinimum,			//  value       extendData            0x00 - 0xFF / 0xFFFF
										//  NOTE: value is packed in to upper byte of extendData (as a word)
			kType_RangeMaximum,			//  value       extendData            0x00 - 0xFF / 0xFFFF
										//  NOTE: value is packed in to lower byte of extendData (as a word)
			// if(Operation == Set/Clear)
			kType_SetClear,				//  select      OperationParameter    Set - Clear
		kType_Data,
		kType_UserSelect,				//  select      UserSelectEnable      Off - On
		kType_UserSelectMinimumDisp,	//  value       UserSelectMinimumDisplay  Off - On
		kType_UserSelectMinimum,		//  value       UserSelectMinimum     0 - 1
		kType_UserSelectBCD,			//  select      UserSelectBCD         Off - On
		kType_CopyPrevious,				//  select      LinkCopyPreviousValue Off - On
		kType_Prefill,					//  select      UserSelectPrefill     None - FF - 00 - 01
		kType_ByteLength,				//  value       BytesUsed             1 - 4
		kType_Endianness,				//  select      Endianness            Normal - Swap
		kType_LocationType,				//  select      LocationType          Normal - Region - Mapped Memory - EEPROM - Relative Address
			// if(LocationType != Region && LocationType != Relative Address)
			kType_CPU,					//  value       LocationParameter     0 - 31
			// if(LocationType == Region)
			kType_Region,				//  select      LocationParameter     CPU1 - CPU2 - CPU3 - CPU4 - CPU5 - CPU6 - CPU7 -
										//                                    CPU8 - GFX1 - GFX2 - GFX3 - GFX4 - GFX5 - GFX6 -
										//                                    GFX7 - GFX8 - PROMS - SOUND1 - SOUND2 - SOUND3 -
										//                                    SOUND4 - SOUND5 - SOUND6 - SOUND7 - SOUND8 -
										//                                    USER1 - USER2 - USER3 - USER4 - USER5 - USER6 -
										//                                    USER7
										//  NOTE: old format doesn't support USER8 - PLDS
			// if(LocationType == Relative Address)
			kType_PackedCPU,			//  value       LocationParameter     0 - 7
										//  NOTE: packed in to upper three bits of LocationParameter
			kType_PackedSize,			//  value       LocationParameter     1 - 4
										//  NOTE: packed in to lower two bits of LocationParameter
			kType_AddressIndex,			//  value       extendData            -0x80000000 - 0x7FFFFFFF
		kType_Address,

		kType_Return,
		kType_Divider,

		kType_Max
	};

	int					i;
	int					sel				= selection - 1;
	int					total			= 0;
	UINT8				dirty			= 0;
	static UINT8		editActive		= 0;
	const char			** menuItem;
	const char			** menuSubItem;
	char				* flagBuf;
	char				** extendDataBuf;		// FFFFFFFF (-80000000)
	char				** addressBuf;			// FFFFFFFF
	char				** dataBuf;				// 80000000 (-2147483648)
	char				** typeBuf;				// FFFFFFFF
	char				** watchSizeBuf;		// FF
	char				** watchSkipBuf;		// FF
	char				** watchPerLineBuf;		// FF
	char				** watchAddValueBuf;	// FF

	menu_item_info		* info = NULL;
	CheatAction			* action		= NULL;
	astring				* tempString1	= astring_alloc();		// activation key 1
	astring				* tempString2	= astring_alloc();		// activation key 2

	if(!entry)
		return 0;

	if(menuItemInfoLength < (kType_Max * entry->actionListLength) + 2)
	{
		menuItemInfoLength = (kType_Max * entry->actionListLength) + 2;

		menuItemInfo = realloc(menuItemInfo, menuItemInfoLength * sizeof(menu_item_info));
	}

	request_strings((kType_Max * entry->actionListLength) + 2, 8 * entry->actionListLength, 32, 0);

	menuItem			= menu_strings.main_list;
	menuSubItem			= menu_strings.sub_list;
	flagBuf				= menu_strings.flag_list;
	extendDataBuf		= &menu_strings.main_strings[entry->actionListLength * 0];
	addressBuf			= &menu_strings.main_strings[entry->actionListLength * 1];
	dataBuf				= &menu_strings.main_strings[entry->actionListLength * 2];
	typeBuf				= &menu_strings.main_strings[entry->actionListLength * 3];
	watchSizeBuf		= &menu_strings.main_strings[entry->actionListLength * 4];	// these fields are wasteful
	watchSkipBuf		= &menu_strings.main_strings[entry->actionListLength * 5];	// but the alternative is even more ugly
	watchPerLineBuf		= &menu_strings.main_strings[entry->actionListLength * 6];	//
	watchAddValueBuf	= &menu_strings.main_strings[entry->actionListLength * 7];	//

	memset(flagBuf, 0, (kType_Max * entry->actionListLength) + 2);

	/********** MENU CONSTRUCTION **********/
	/* create menu items */
	for(i = 0; i < entry->actionListLength; i++)
	{
		CheatAction	* traverse = &entry->actionList[i];

		UINT32	type					= EXTRACT_FIELD(traverse->type, Type);
		UINT32	typeParameter			= EXTRACT_FIELD(traverse->type, TypeParameter);
		UINT32	operation				= EXTRACT_FIELD(traverse->type, Operation) | EXTRACT_FIELD(traverse->type, OperationExtend) << 2;
		UINT32	operationParameter		= EXTRACT_FIELD(traverse->type, OperationParameter);
		UINT32	locationType			= EXTRACT_FIELD(traverse->type, LocationType);
		UINT32	locationParameter		= EXTRACT_FIELD(traverse->type, LocationParameter);

		/* create item if label-select, extend comment or condition */
		if(!i)
		{
			/* do name field */
			menuItemInfo[total].sub_cheat = i;
			menuItemInfo[total].field_type = kType_Name;
			menuItem[total] = "Name";

			if(entry->name)
				menuSubItem[total++] = entry->name;
			else
				menuSubItem[total++] = "(none)";
		}
		else
		{
			/* do label name field */
			menuItemInfo[total].sub_cheat = i;
			menuItemInfo[total].field_type = kType_ExtendName;
			menuItem[total] = "Name";

			if(traverse->optionalName)
				menuSubItem[total++] = traverse->optionalName;
			else
				menuSubItem[total++] = "(none)";
		}

		if(!i)
		{
			/* do comment field */
			menuItemInfo[total].sub_cheat = i;
			menuItemInfo[total].field_type = kType_Comment;
			menuItem[total] = "Comment";

			if(entry->comment)
				menuSubItem[total++] = entry->comment;
			else
				menuSubItem[total++] = "(none)";
		}

		{
			/* do 1st (previous-selection) activation key field */
			menuItemInfo[total].sub_cheat = i;
			menuItemInfo[total].field_type = kType_ActivationKey1;

			if(locationType == kLocation_Custom && locationParameter == kCustomLocation_Select)
				menuItem[total] = "Activation Key - Prev";
			else
				menuItem[total] = "Activation Key - 1st";

			if(entry->flags & kCheatFlag_HasActivationKey1)
				menuSubItem[total++] = astring_c(input_code_name(tempString1, entry->activationKey1));
			else
				menuSubItem[total++] = "(none)";
		}

		{
			/* do 2nd (next-selection) activation key field */
			menuItemInfo[total].sub_cheat = i;
			menuItemInfo[total].field_type = kType_ActivationKey2;

			if(locationType == kLocation_Custom && locationParameter == kCustomLocation_Select)
				menuItem[total] = "Activation Key - Next";
			else
				menuItem[total] = "Activation Key - 2nd";

			if(entry->flags & kCheatFlag_HasActivationKey2)
				menuSubItem[total++] = astring_c(input_code_name(tempString2, entry->activationKey2));
			else
				menuSubItem[total++] = "(none)";
		}

		{
			/* do type code field */
			menuItemInfo[total].sub_cheat = i;
			menuItemInfo[total].field_type = kType_Code;
			menuItem[total] = "Code";

			sprintf(typeBuf[i], "%8.8X", traverse->type);
			menuSubItem[total++] = typeBuf[i];
		}

		{
			/* do link field */
			menuItemInfo[total].sub_cheat = i;
			menuItemInfo[total].field_type = kType_Link;
			menuItem[total] = "Link";
			menuSubItem[total++] = TEST_FIELD(traverse->type, LinkEnable) ? "On" : "Off";
		}

		{
			/* do link extension field */
			menuItemInfo[total].sub_cheat = i;
			menuItemInfo[total].field_type = kType_LinkExtension;
			menuItem[total] = "Link Extension";
			menuSubItem[total++] = TEST_FIELD(traverse->type, LinkExtension) ? "On" : "Off";
		}

		{
			/* do type field */
			menuItemInfo[total].sub_cheat = i;
			menuItemInfo[total].field_type = kType_Type;
			menuItem[total] = "Type";

			if(locationType == kLocation_Custom && locationParameter == kCustomLocation_Comment)
				menuSubItem[total++] = kTypeNames[4];
			else if(locationType == kLocation_Custom && locationParameter == kCustomLocation_Select)
				menuSubItem[total++] = kTypeNames[5];
			else
				menuSubItem[total++] = kTypeNames[type & 3];
		}

		{
			/* do one shot field */
			menuItemInfo[total].sub_cheat = i;
			menuItemInfo[total].field_type = kType_OneShot;
			menuItem[total] = "One Shot";
			menuSubItem[total++] = TEST_FIELD(traverse->type, OneShot) ? "On" : "Off";
		}

		{
			/* do restore previous value field */
			menuItemInfo[total].sub_cheat = i;
			menuItemInfo[total].field_type = kType_RestorePreviousValue;
			menuItem[total] = "Restore Previous Value";
			menuSubItem[total++] = TEST_FIELD(traverse->type, RestorePreviousValue) ? "On" : "Off";
		}

		switch(type)
		{
			default:
			{
				/* do delay field */
				menuItemInfo[total].sub_cheat = i;
				menuItemInfo[total].field_type = kType_Delay;

				if(TEST_FIELD(traverse->type, OneShot) && TEST_FIELD(traverse->type, RestorePreviousValue) && EXTRACT_FIELD(traverse->type, TypeParameter))
					menuItem[total] = "Keep";
				else
					menuItem[total] = "Delay";

				menuSubItem[total++] = kNumbersTable[typeParameter];
			}
			break;

			case kType_IgnoreIfDecrementing:
			{
				/* do ignore decrement by field */
				menuItemInfo[total].sub_cheat = i;
				menuItemInfo[total].field_type = kType_IgnoreDecrementBy;
				menuItem[total] = "Ignore Decrement By";
				menuSubItem[total++] = kNumbersTable[typeParameter];
			}
			break;

			case kType_Watch:
			{
				/* do watch size field */
				sprintf(watchSizeBuf[i], "%d", (traverse->originalDataField & 0xFF) + 1);

				menuItemInfo[total].sub_cheat = i;
				menuItemInfo[total].field_type = kType_WatchSize;
				menuItem[total] = "Watch Size";
				menuSubItem[total++] = watchSizeBuf[i];
			}

			{
				/* do watch skip field */
				sprintf(watchSkipBuf[i], "%d", (traverse->data >> 8) & 0xFF);

				menuItemInfo[total].sub_cheat = i;
				menuItemInfo[total].field_type = kType_WatchSkip;
				menuItem[total] = "Watch Skip";
				menuSubItem[total++] = watchSkipBuf[i];
			}

			{
				/* do watch per line field */
				sprintf(watchPerLineBuf[i], "%d", (traverse->data >> 16) & 0xFF);

				menuItemInfo[total].sub_cheat = i;
				menuItemInfo[total].field_type = kType_WatchPerLine;
				menuItem[total] = "Watch Per Line";
				menuSubItem[total++] = watchPerLineBuf[i];
			}

			{
				/* do watch add value field */
				INT8 temp = (traverse->data >> 24) & 0xFF;

				if(temp < 0)
					sprintf(watchAddValueBuf[i], "-%.2X", -temp);
				else
					sprintf(watchAddValueBuf[i], "%.2X", temp);

				menuItemInfo[total].sub_cheat = i;
				menuItemInfo[total].field_type = kType_WatchAddValue;
				menuItem[total] = "Watch Add Value";
				menuSubItem[total++] = watchAddValueBuf[i];
			}

			{
				/* do watch format field */
				menuItemInfo[total].sub_cheat = i;
				menuItemInfo[total].field_type = kType_WatchFormat;
				menuItem[total] = "Watch Format";
				menuSubItem[total++] = kWatchDisplayTypeStringList[(typeParameter >> 0) & 0x03];
			}

			{
				/* do watch label field */
				menuItemInfo[total].sub_cheat = i;
				menuItemInfo[total].field_type = kType_WatchLabel;
				menuItem[total] = "Watch Label";
				menuSubItem[total++] = typeParameter >> 2 & 0x01 ? "On" : "Off";
			}
			break;
		}

		{
			/* do operation field */
			menuItemInfo[total].sub_cheat = i;
			menuItemInfo[total].field_type = kType_Operation;
			menuItem[total] = "Operation";

			menuSubItem[total++] = kOperationNames[operation];
		}

		{
			/* do operation extend field */
			menuItemInfo[total].sub_cheat = i;
			menuItemInfo[total].field_type = kType_OperationExtend;
			menuItem[total] = "Operation Extend";
			menuSubItem[total++] = TEST_FIELD(traverse->type, OperationExtend) ? "On" : "Off";
		}

		if(locationType != kLocation_IndirectIndexed)
		{
			switch(operation)
			{
				default:
				{
					/* do mask field */
					int numChars;

					if(traverse->flags & kActionFlag_IndexAddress)
					{
						menuItemInfo[total].extra_data = 0xFFFFFFFF;
						numChars = 8;
					}
					else
					{
						menuItemInfo[total].extra_data = kCheatSizeMaskTable[EXTRACT_FIELD(traverse->type, BytesUsed)];
						numChars = kCheatSizeDigitsTable[EXTRACT_FIELD(traverse->type, BytesUsed)];
					}

					sprintf(extendDataBuf[i], "%.*X", numChars, traverse->extendData & kCheatSizeMaskTable[EXTRACT_FIELD(traverse->type, BytesUsed)]);

					menuItemInfo[total].sub_cheat = i;
					menuItemInfo[total].field_type = kType_WriteMask;
					menuItem[total] = "Mask";
					menuSubItem[total++] = extendDataBuf[i];
				}
				break;

				case kOperation_AddSubtract:
				{
					{
						/* do add/subtract field */
						menuItemInfo[total].sub_cheat = i;
						menuItemInfo[total].field_type = kType_AddSubtract;
						menuItem[total] = "Add/Subtract";
						menuSubItem[total++] = kAddSubtractNames[operationParameter];
					}

					if(operationParameter)
					{
						/* do subtract minimum field */
						sprintf(extendDataBuf[i], "%.8X", traverse->extendData);

						menuItemInfo[total].sub_cheat = i;
						menuItemInfo[total].field_type = kType_SubtractMinimum;
						menuItem[total] = "Minimum Boundary";
						menuSubItem[total++] = extendDataBuf[i];
					}
					else
					{
						/* do add maximum field */
						sprintf(extendDataBuf[i], "%.8X", traverse->extendData);

						menuItemInfo[total].sub_cheat = i;
						menuItemInfo[total].field_type = kType_AddMaximum;
						menuItem[total] = "Maximum Boundary";
						menuSubItem[total++] = extendDataBuf[i];
					}
				}
				break;

				case kOperation_ForceRange:
				{
					{
						/* do range minimum field */
						if(!EXTRACT_FIELD(traverse->type, BytesUsed))
							sprintf(extendDataBuf[i], "%.2X", (traverse->extendData >> 8) & 0xFF);
						else
							sprintf(extendDataBuf[i], "%.4X", (traverse->extendData >> 16) & 0xFFFF);

						menuItemInfo[total].sub_cheat = i;
						menuItemInfo[total].field_type = kType_RangeMinimum;
						menuItem[total] = "Range Minimum";
						menuSubItem[total++] = extendDataBuf[i];
					}

					{
						/* do range maximum field */
						if(!EXTRACT_FIELD(traverse->type, BytesUsed))
						{
							sprintf(extendDataBuf[i] + 3, "%.2X", (traverse->extendData >> 0) & 0xFF);
							menuSubItem[total] = extendDataBuf[i] + 3;
						}
						else
						{
							sprintf(extendDataBuf[i] + 7, "%.4X", (traverse->extendData >> 0) & 0xFFFF);
							menuSubItem[total] = extendDataBuf[i] + 7;
						}

						menuItemInfo[total].sub_cheat = i;
						menuItemInfo[total].field_type = kType_RangeMaximum;
						menuItem[total++] = "Range Maximum";
					}
				}
				break;

				case kOperation_SetOrClearBits:
				{
					/* do set/clear field */
					menuItemInfo[total].sub_cheat = i;
					menuItemInfo[total].field_type = kType_SetClear;
					menuItem[total] = "Set/Clear";
					menuSubItem[total++] = kSetClearNames[operationParameter];
				}
				break;
			}
		}

		{
			/* do data field */
			sprintf(dataBuf[i], "%.*X (%d)",
					(int)kCheatSizeDigitsTable[EXTRACT_FIELD(traverse->type, BytesUsed)],
					traverse->originalDataField,
					traverse->originalDataField);

			menuItemInfo[total].sub_cheat = i;
			menuItemInfo[total].field_type = kType_Data;
			menuItemInfo[total].extra_data = kCheatSizeMaskTable[EXTRACT_FIELD(traverse->type, BytesUsed)];
			menuItem[total] = "Data";
			menuSubItem[total++] = dataBuf[i];
		}

		{
			/* do user select field */
			menuItemInfo[total].sub_cheat = i;
			menuItemInfo[total].field_type = kType_UserSelect;
			menuItem[total] = "User Select";
			menuSubItem[total++] = EXTRACT_FIELD(traverse->type, UserSelectEnable) ? "On" : "Off";
		}

		{
			/* do user select minimum displayed value field */
			menuItemInfo[total].sub_cheat = i;
			menuItemInfo[total].field_type = kType_UserSelectMinimumDisp;
			menuItem[total] = "Minimum Displayed Value";
			menuSubItem[total++] = kNumbersTable[EXTRACT_FIELD(traverse->type, UserSelectMinimumDisplay)];
		}

		{
			/* do user select minimum value field */
			menuItemInfo[total].sub_cheat = i;
			menuItemInfo[total].field_type = kType_UserSelectMinimum;
			menuItem[total] = "Minimum Value";
			menuSubItem[total++] = kNumbersTable[EXTRACT_FIELD(traverse->type, UserSelectMinimum)];
		}

		{
			/* do user select BCD field */
			menuItemInfo[total].sub_cheat = i;
			menuItemInfo[total].field_type = kType_UserSelectBCD;
			menuItem[total] = "BCD";
			menuSubItem[total++] = TEST_FIELD(traverse->type, UserSelectBCD) ? "On" : "Off";
		}

		{
			/* do copy previous value field */
			menuItemInfo[total].sub_cheat = i;
			menuItemInfo[total].field_type = kType_CopyPrevious;
			menuItem[total] = "Copy Previous Value";
			menuSubItem[total++] = TEST_FIELD(traverse->type, LinkCopyPreviousValue) ? "On" : "Off";
		}

		{
			/* do prefill field */
			menuItemInfo[total].sub_cheat = i;
			menuItemInfo[total].field_type = kType_Prefill;
			menuItem[total] = "Prefill";
			menuSubItem[total++] = kPrefillNames[EXTRACT_FIELD(traverse->type, Prefill)];
		}

		{
			/* do byte length field */
			menuItemInfo[total].sub_cheat = i;
			menuItemInfo[total].field_type = kType_ByteLength;
			menuItem[total] = "Byte Length";
			menuSubItem[total++] = kByteSizeStringList[EXTRACT_FIELD(traverse->type, BytesUsed)];
		}

		{
			/* do endianness field */
			menuItemInfo[total].sub_cheat = i;
			menuItemInfo[total].field_type = kType_Endianness;
			menuItem[total] = "Endianness";
			menuSubItem[total++] = kEndiannessNames[EXTRACT_FIELD(traverse->type, Endianness)];
		}

		{
			/* do location type field */
			menuItemInfo[total].sub_cheat = i;
			menuItemInfo[total].field_type = kType_LocationType;
			menuItem[total] = "Location";

			if(locationType == kLocation_Custom)
				/* "Comment", "EEPROM", "Select" */
				menuSubItem[total++] = kCustomLocationNames[locationParameter];
			else
				/* "Normal", "Region", "Mapped Memory", "Relative Address" */
				menuSubItem[total++] = kLocationNames[locationType];
		}

		if(locationType != kLocation_IndirectIndexed)
		{
			if(locationType != kLocation_MemoryRegion)
			{
				/* do cpu field */
				menuItemInfo[total].sub_cheat = i;
				menuItemInfo[total].field_type = kType_CPU;
				menuItem[total] = "CPU";
				menuSubItem[total++] = kNumbersTable[locationParameter];
			}
			else
			{
				/* do region field */
				menuItemInfo[total].sub_cheat = i;
				menuItemInfo[total].field_type = kType_Region;
				menuItem[total] = "Region";
				menuSubItem[total++] = kRegionNames[locationParameter];
			}
		}
		else
		{
			{
				/* do packed CPU field */
				menuItemInfo[total].sub_cheat = i;
				menuItemInfo[total].field_type = kType_PackedCPU;
				menuItem[total] = "CPU";
				menuSubItem[total++] = kNumbersTable[EXTRACT_FIELD(traverse->type, LocationParameterCPU)];
			}

			{
				/* do packed size field */
				menuItemInfo[total].sub_cheat = i;
				menuItemInfo[total].field_type = kType_PackedSize;
				menuItem[total] = "Address Size";
				menuSubItem[total++] = kNumbersTable[(locationParameter & 3) + 1];
			}

			{
				/* do address index field */
				if(traverse->extendData & 0x80000000)
				{
					/* swap if negative */
					int temp = traverse->extendData;

					temp = -temp;

					sprintf(extendDataBuf[i], "-%.8X", temp);
				}
				else
					sprintf(extendDataBuf[i], "%.8X", traverse->extendData);

				menuItemInfo[total].sub_cheat = i;
				menuItemInfo[total].field_type = kType_AddressIndex;
				menuItem[total] = "Address Index";
				menuSubItem[total++] = extendDataBuf[i];
			}
		}

		{
			/* do address field */
			cpu_region_info *info = get_cpu_info(0);

			switch(EXTRACT_FIELD(traverse->type, LocationType))
			{
				case kLocation_Standard:
				case kLocation_HandlerMemory:
				case kLocation_MemoryRegion:
				case kLocation_IndirectIndexed:
					menuItemInfo[total].extra_data = info->addressMask;
					break;

				default:
					menuItemInfo[total].extra_data = 0xFFFFFFFF;
			}

			sprintf(addressBuf[i], "%.*X", get_address_length(EXTRACT_FIELD(traverse->type, LocationParameter)), traverse->address);

			menuItemInfo[total].sub_cheat = i;
			menuItemInfo[total].field_type = kType_Address;
			menuItem[total] = "Address";
			menuSubItem[total++] = addressBuf[i];
		}

		if(i < entry->actionListLength - 1)
		{
			/* do divider */
			menuItemInfo[total].sub_cheat = i;
			menuItemInfo[total].field_type = kType_Divider;
			menuItem[total] = MENU_SEPARATOR_ITEM;
			menuSubItem[total++] = NULL;
		}
	}

	/* ##### RETURN ##### */
	menuItemInfo[total].sub_cheat = 0;
	menuItemInfo[total].field_type = kType_Return;
	menuItem[total] = "Return to Prior Menu";
	menuSubItem[total++] = NULL;

	/* ##### TERMINATE ARREY ##### */
	menuItemInfo[total].sub_cheat = 0;
	menuItemInfo[total].field_type = kType_Return;
	menuItem[total] = NULL;
	menuSubItem[total] = NULL;

	/* adjust cursor position */
	if(sel < 0)
		sel = 0;
	if(sel >= total)
		sel = total - 1;

	info = &menuItemInfo[sel];
	action = &entry->actionList[info->sub_cheat];

	/* higlighted sub-item if edit mode */
	if(editActive)
		flagBuf[sel] = 1;

	/* draw it */
	old_style_menu(menuItem, menuSubItem, flagBuf, sel, 0);

	/********** KEY HANDLING **********/
	if(ui_pressed_repeat_throttle(machine, IPT_UI_LEFT, kHorizontalSlowKeyRepeatRate))
	{
		editActive = 0;
		dirty = 1;

		switch(info->fieldType)
		{
/*          case kType_ActivationKey1:
            case kType_ActivationKey2:
                if(info->fieldType == kType_ActivationKey1)
                {
                    entry->activationKey1--;

                if(entry->activationKey1 < 0)
                        entry->activationKey1 = __code_max - 1;
                    if(entry->activationKey1 >= __code_max)
                        entry->activationKey1 = 0;

                    entry->flags |= kCheatFlag_HasActivationKey1;
                }
                else
                {
                    entry->activationKey2--;

                    if(entry->activationKey2 < 0)
                        entry->activationKey2 = __code_max - 1;
                    if(entry->activationKey2 >= __code_max)
                        entry->activationKey2 = 0;

                    entry->flags |= kCheatFlag_HasActivationKey2;
                }
                break;
*/
			case kType_Link:
				TOGGLE_MASK_FIELD(action->type, LinkEnable);
				break;

			case kType_LinkExtension:
				TOGGLE_MASK_FIELD(action->type, LinkExtension);
				break;

			case kType_Type:
			{
				UINT8 handled = 0;

				if(EXTRACT_FIELD(action->type, LocationType) == kLocation_Custom)
				{
					UINT32 locationParameter = EXTRACT_FIELD(action->type, LocationParameter);

					if(locationParameter == kCustomLocation_Comment)
					{
						/* "Comment" -> "Watch" */
						SET_FIELD(action->type, LocationParameter, 0);
						SET_FIELD(action->type, LocationType, kLocation_Standard);
						SET_FIELD(action->type, Type, kType_Watch);
						SET_FIELD(action->type, Operation, kOperation_None);

						handled = 1;
					}
					else if(locationParameter == kCustomLocation_Select)
					{
						/* "Select" -> "Comment" */
						SET_FIELD(action->type, LocationParameter, kCustomLocation_Comment);
						SET_FIELD(action->type, LocationType, kLocation_Custom);
						SET_FIELD(action->type, Type, 0);

						handled = 1;
					}
				}

				if(!handled)
				{
					UINT32 type = EXTRACT_FIELD(action->type, Type);

					if(type == kType_NormalOrDelay)
					{
						/* "Normal/Delay" -> "Select" */
						SET_FIELD(action->type, LocationType, kLocation_Custom);
						SET_FIELD(action->type, LocationParameter, kCustomLocation_Select);
						SET_FIELD(action->type, Type, 0);
					}
					else
						SET_FIELD(action->type, Type, type - 1);
				}
			}
			break;

			case kType_OneShot:
				TOGGLE_MASK_FIELD(action->type, OneShot);
				break;

			case kType_RestorePreviousValue:
				TOGGLE_MASK_FIELD(action->type, RestorePreviousValue);
				break;

			case kType_Delay:
			{
				UINT32 delay = (EXTRACT_FIELD(action->type, TypeParameter) - 1) & 7;

				SET_FIELD(action->type, TypeParameter, delay);
			}
			break;

			case kType_WatchSize:
				action->originalDataField = (action->originalDataField & 0xFFFFFF00) | ((action->originalDataField - 0x00000001) & 0x000000FF);
				action->data = action->originalDataField;
				break;

			case kType_WatchSkip:
				action->originalDataField = (action->originalDataField & 0xFFFF00FF) | ((action->originalDataField - 0x00000100) & 0x0000FF00);
				action->data = action->originalDataField;
				break;

			case kType_WatchPerLine:
				action->originalDataField = (action->originalDataField & 0xFF00FFFF) | ((action->originalDataField - 0x00010000) & 0x00FF0000);
				action->data = action->originalDataField;
				break;

			case kType_WatchAddValue:
				action->originalDataField = (action->originalDataField & 0x00FFFFFF) | ((action->originalDataField - 0x01000000) & 0xFF000000);
				action->data = action->originalDataField;
				break;

			case kType_WatchFormat:
			{
				UINT32 typeParameter = EXTRACT_FIELD(action->type, TypeParameter);

				typeParameter = (typeParameter & 0xFFFFFFFC) | ((typeParameter - 0x00000001) & 0x0000003);
				SET_FIELD(action->type, TypeParameter, typeParameter);
			}
			break;

			case kType_WatchLabel:
				SET_FIELD(action->type, TypeParameter, EXTRACT_FIELD(action->type, TypeParameter) ^ 0x00000004);
				break;

			case kType_Operation:
				if(EXTRACT_FIELD(action->type, Operation) == kOperation_WriteMask)
				{
					if(EXTRACT_FIELD(action->type, LocationType) == kLocation_IndirectIndexed)
						SET_FIELD(action->type, Operation, kOperation_SetOrClearBits);
					else
						SET_FIELD(action->type, Operation, kOperation_ForceRange);
				}
				else
					SET_FIELD(action->type, Operation, EXTRACT_FIELD(action->type, Operation) - 1);
				break;

			case kType_OperationExtend:
				TOGGLE_MASK_FIELD(action->type, OperationExtend);
				break;

			case kType_WriteMask:
				action->extendData -= 1;
				action->extendData &= info->extraData;
				break;

			case kType_RangeMinimum:
				if(!EXTRACT_FIELD(action->type, BytesUsed))
					action->extendData = (action->extendData & 0xFFFF00FF) | ((action->extendData - 0x00000100) & 0x0000FF00);
				else
					action->extendData = (action->extendData & 0x0000FFFF) | ((action->extendData - 0x00010000) & 0xFFFF0000);
				break;

			case kType_RangeMaximum:
				if(!EXTRACT_FIELD(action->type, BytesUsed))
					action->extendData = (action->extendData & 0xFFFFFF00) | ((action->extendData - 0x00000001) & 0x000000FF);
				else
					action->extendData = (action->extendData & 0xFFFF0000) | ((action->extendData - 0x00000001) & 0x0000FFFF);
				break;

			case kType_SubtractMinimum:
			case kType_AddMaximum:
			case kType_AddressIndex:
				action->extendData -= 1;
				break;

			case kType_AddSubtract:
			case kType_SetClear:
				TOGGLE_MASK_FIELD(action->type, OperationParameter);
				break;

			case kType_Data:
				action->originalDataField -= 1;
				action->originalDataField &= info->extraData;
				action->data = action->originalDataField;
				break;

			case kType_UserSelect:
				TOGGLE_MASK_FIELD(action->type, UserSelectEnable);
				break;

			case kType_UserSelectMinimumDisp:
				TOGGLE_MASK_FIELD(action->type, UserSelectMinimumDisplay);
				break;

			case kType_UserSelectMinimum:
				TOGGLE_MASK_FIELD(action->type, UserSelectMinimum);
				break;

			case kType_UserSelectBCD:
				TOGGLE_MASK_FIELD(action->type, UserSelectBCD);
				break;

			case kType_CopyPrevious:
				TOGGLE_MASK_FIELD(action->type, LinkCopyPreviousValue);
				break;

			case kType_Prefill:
			{
				UINT32 prefill = (EXTRACT_FIELD(action->type, Prefill) - 1) & 3;

				SET_FIELD(action->type, Prefill, prefill);
			}
			break;

			case kType_ByteLength:
			{
				UINT32 length = (EXTRACT_FIELD(action->type, BytesUsed) - 1) & 3;

				SET_FIELD(action->type, BytesUsed, length);
			}
			break;

			case kType_Endianness:
				TOGGLE_MASK_FIELD(action->type, Endianness);
				break;

			case kType_LocationType:
			{
				UINT32 locationType = EXTRACT_FIELD(action->type, LocationType);

				switch(locationType)
				{
					case kLocation_Standard:
						/* "Normal" -> "Relative Address" */
						SET_FIELD(action->type, LocationType, kLocation_IndirectIndexed);
						SET_FIELD(action->type, LocationParameter, 0);
						break;

					case kLocation_Custom:
						/* "EEPROM" -> "Mapped Memory" */
						SET_FIELD(action->type, LocationType, kLocation_HandlerMemory);
						SET_FIELD(action->type, LocationParameter, 0);
						break;

					case kLocation_IndirectIndexed:
						/* "Relative Address" -> "EEPROM" */
						SET_FIELD(action->type, LocationType, kLocation_Custom);
						SET_FIELD(action->type, LocationParameter, kCustomLocation_EEPROM);
						break;

					default:
						locationType--;
						SET_FIELD(action->type, LocationType, locationType);
				}
			}
			break;

			case kType_CPU:
			case kType_Region:
			{
				UINT32 locationParameter = EXTRACT_FIELD(action->type, LocationParameter);

				locationParameter = (locationParameter - 1) & 31;

				SET_FIELD(action->type, LocationParameter, locationParameter);
			}
			break;

			case kType_PackedCPU:
			{
				UINT32 locationParameter = EXTRACT_FIELD(action->type, LocationParameterCPU);

				locationParameter = (locationParameter - 1) & 7;

				SET_FIELD(action->type, LocationParameterCPU, locationParameter);
			}
			break;

			case kType_PackedSize:
			{
				UINT32 locationParameter = EXTRACT_FIELD(action->type, LocationParameterOption);

				locationParameter = (locationParameter - 1) & 3;

				SET_FIELD(action->type, LocationParameterOption, locationParameter);
			}
			break;

			case kType_Address:
				action->address -= 1;
				action->address &= info->extraData;
				break;

			case kType_Return:
			case kType_Divider:
				break;
		}
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_RIGHT, kHorizontalSlowKeyRepeatRate))
	{
		editActive = 0;
		dirty = 1;

		switch(info->fieldType)
		{
/*          case kType_ActivationKey1:
            case kType_ActivationKey2:
                if(info->fieldType == kType_ActivationKey1)
                {
                    entry->activationKey1++;

                    if(entry->activationKey1 < 0)
                        entry->activationKey1 = __code_max - 1;
                    if(entry->activationKey1 >= __code_max)
                        entry->activationKey1 = 0;

                    entry->flags |= kCheatFlag_HasActivationKey1;
                }
                else
                {
                    entry->activationKey2++;

                    if(entry->activationKey2 < 0)
                        entry->activationKey2 = __code_max - 1;
                    if(entry->activationKey2 >= __code_max)
                        entry->activationKey2 = 0;

                    entry->flags |= kCheatFlag_HasActivationKey2;
                }
                break;
*/
			case kType_Link:
				TOGGLE_MASK_FIELD(action->type, LinkEnable);
				break;

			case kType_LinkExtension:
				TOGGLE_MASK_FIELD(action->type, LinkExtension);
				break;

			case kType_Type:
			{
				UINT8 handled = 0;

				if(EXTRACT_FIELD(action->type, LocationType) == kLocation_Custom)
				{
					UINT32 locationParameter = EXTRACT_FIELD(action->type, LocationParameter);

					if(locationParameter == kCustomLocation_Comment)
					{
						/* "Comment" -> "Select" */
						SET_FIELD(action->type, LocationParameter, kCustomLocation_Select);
						SET_FIELD(action->type, LocationType, kLocation_Custom);
						SET_FIELD(action->type, Type, 0);

						handled = 1;
					}
				}
				else if((EXTRACT_FIELD(action->type, LocationType) == kLocation_Custom) &&
						(EXTRACT_FIELD(action->type, LocationParameter) == kCustomLocation_Select))
				{
					/* "Select" -> "Normal/Delay" */
					SET_FIELD(action->type, LocationParameter, 0);
					SET_FIELD(action->type, LocationType, kLocation_Standard);
					SET_FIELD(action->type, Type, 0);

					handled = 1;
				}

				if(!handled)
				{
					UINT32 type = EXTRACT_FIELD(action->type, Type);

					if(type == kType_Watch)
					{
						/* "Watch" -> "Comment" */
						SET_FIELD(action->type, LocationParameter, kCustomLocation_Comment);
						SET_FIELD(action->type, LocationType, kLocation_Custom);
						SET_FIELD(action->type, Type, 0);
					}
					else
						SET_FIELD(action->type, Type, type + 1);
				}
			}
			break;

			case kType_OneShot:
				TOGGLE_MASK_FIELD(action->type, OneShot);
				break;

			case kType_RestorePreviousValue:
				TOGGLE_MASK_FIELD(action->type, RestorePreviousValue);
				break;

			case kType_Delay:
			{
				UINT32 delay = (EXTRACT_FIELD(action->type, TypeParameter) + 1) & 7;

				SET_FIELD(action->type, TypeParameter, delay);
			}
			break;

			case kType_WatchSize:
				action->originalDataField = (action->originalDataField & 0xFFFFFF00) | ((action->originalDataField + 0x00000001) & 0x000000FF);
				action->data = action->originalDataField;
				break;

			case kType_WatchSkip:
				action->originalDataField = (action->originalDataField & 0xFFFF00FF) | ((action->originalDataField + 0x00000100) & 0x0000FF00);
				action->data = action->originalDataField;
				break;

			case kType_WatchPerLine:
				action->originalDataField = (action->originalDataField & 0xFF00FFFF) | ((action->originalDataField + 0x00010000) & 0x00FF0000);
				action->data = action->originalDataField;
				break;

			case kType_WatchAddValue:
				action->originalDataField = (action->originalDataField & 0x00FFFFFF) | ((action->originalDataField + 0x01000000) & 0xFF000000);
				action->data = action->originalDataField;
				break;

			case kType_WatchFormat:
			{
				UINT32 typeParameter = EXTRACT_FIELD(action->type, TypeParameter);

				typeParameter = (typeParameter & 0xFFFFFFFC) | ((typeParameter + 0x00000001) & 0x0000003);
				SET_FIELD(action->type, TypeParameter, typeParameter);
			}
			break;

			case kType_WatchLabel:
				SET_FIELD(action->type, TypeParameter, EXTRACT_FIELD(action->type, TypeParameter) ^ 0x00000004);
				break;

			case kType_Operation:
			{
				CLEAR_MASK_FIELD(action->type, OperationExtend);

				if(EXTRACT_FIELD(action->type, LocationType) == kLocation_IndirectIndexed)
				{
					if(EXTRACT_FIELD(action->type, Type) >= kOperation_SetOrClearBits)
						SET_FIELD(action->type, Operation, 0);
					else
						SET_FIELD(action->type, Operation, EXTRACT_FIELD(action->type, Operation) + 1);
				}
				else if(EXTRACT_FIELD(action->type, Operation) >= kOperation_ForceRange)
					SET_FIELD(action->type, Operation, 0);
				else
					SET_FIELD(action->type, Operation, EXTRACT_FIELD(action->type, Operation) + 1);
			}
			break;

			case kType_OperationExtend:
				TOGGLE_MASK_FIELD(action->type, OperationExtend);
				break;

			case kType_WriteMask:
				action->extendData += 1;
				action->extendData &= info->extraData;
				break;

			case kType_RangeMinimum:
				if(!EXTRACT_FIELD(action->type, BytesUsed))
					action->extendData = (action->extendData & 0xFFFF00FF) | ((action->extendData + 0x00000100) & 0x0000FF00);
				else
					action->extendData = (action->extendData & 0x0000FFFF) | ((action->extendData + 0x00010000) & 0xFFFF0000);
				break;

			case kType_RangeMaximum:
				if(!EXTRACT_FIELD(action->type, BytesUsed))
					action->extendData = (action->extendData & 0xFFFFFF00) | ((action->extendData + 0x00000001) & 0x000000FF);
				else
					action->extendData = (action->extendData & 0xFFFF0000) | ((action->extendData + 0x00000001) & 0x0000FFFF);
				break;

			case kType_AddressIndex:
			case kType_SubtractMinimum:
			case kType_AddMaximum:
				action->extendData += 1;
				break;

			case kType_AddSubtract:
			case kType_SetClear:
				TOGGLE_MASK_FIELD(action->type, OperationParameter);
				break;

			case kType_Data:
				action->originalDataField += 1;
				action->originalDataField &= info->extraData;
				action->data = action->originalDataField;
				break;

			case kType_UserSelect:
				TOGGLE_MASK_FIELD(action->type, UserSelectEnable);
				break;

			case kType_UserSelectMinimumDisp:
				TOGGLE_MASK_FIELD(action->type, UserSelectMinimumDisplay);
				break;

			case kType_UserSelectMinimum:
				TOGGLE_MASK_FIELD(action->type, UserSelectMinimum);
				break;

			case kType_UserSelectBCD:
				TOGGLE_MASK_FIELD(action->type, UserSelectBCD);
				break;

			case kType_CopyPrevious:
				TOGGLE_MASK_FIELD(action->type, LinkCopyPreviousValue);
				break;

			case kType_Prefill:
			{
				UINT32 prefill = (EXTRACT_FIELD(action->type, Prefill) + 1) & 3;

				SET_FIELD(action->type, Prefill, prefill);
			}
				break;

			case kType_ByteLength:
			{
				UINT32 length = (EXTRACT_FIELD(action->type, BytesUsed) + 1) & 3;

				SET_FIELD(action->type, BytesUsed, length);
			}
			break;

			case kType_Endianness:
				TOGGLE_MASK_FIELD(action->type, Endianness);
				break;

			case kType_LocationType:
			{
				UINT32 locationType = EXTRACT_FIELD(action->type, LocationType);

				switch(locationType)
				{
					case kLocation_HandlerMemory:
						/* "Mapped Memory" -> "EEPROM" */
						SET_FIELD(action->type, LocationType, kLocation_Custom);
						SET_FIELD(action->type, LocationParameter, kCustomLocation_EEPROM);
						break;

					case kLocation_Custom:
						/* "EEPROM" -> "Relative Address" */
						SET_FIELD(action->type, LocationType, kLocation_IndirectIndexed);
						SET_FIELD(action->type, LocationParameter, 0);
						break;

					case kLocation_IndirectIndexed:
						/* "Relative Address" -> "Normal" */
						SET_FIELD(action->type, LocationType, kLocation_Standard);
						SET_FIELD(action->type, LocationParameter, 0);
						break;

					default:
						locationType++;
						SET_FIELD(action->type, LocationType, locationType);
				}
			}
			break;

			case kType_CPU:
			case kType_Region:
			{
				UINT32 locationParameter = EXTRACT_FIELD(action->type, LocationParameter);

				locationParameter = (locationParameter + 1) & 31;

				SET_FIELD(action->type, LocationParameter, locationParameter);
			}
			break;

			case kType_PackedCPU:
			{
				UINT32 locationParameter = EXTRACT_FIELD(action->type, LocationParameterCPU);

				locationParameter = (locationParameter + 1) & 7;

				SET_FIELD(action->type, LocationParameterCPU, locationParameter);
			}
			break;

			case kType_PackedSize:
			{
				UINT32 locationParameter = EXTRACT_FIELD(action->type, LocationParameterOption);

				locationParameter = (locationParameter + 1) & 3;

				SET_FIELD(action->type, LocationParameterOption, locationParameter);
			}
			break;

			case kType_Address:
				action->address += 1;
				action->address &= info->extraData;
				break;

			case kType_Return:
			case kType_Divider:
				break;
		}
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_UP, kVerticalKeyRepeatRate))
	{
		if(--sel < 0)
			sel = total - 1;

		editActive = 0;
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_DOWN, kVerticalKeyRepeatRate))
	{
		if(++sel >= total)
			sel = 0;

		editActive = 0;
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_PAGE_UP, kVerticalKeyRepeatRate))
	{
		sel -= fullMenuPageHeight;

		if(sel < 0)
			sel = 0;

		editActive = 0;
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_PAGE_DOWN, kVerticalKeyRepeatRate))
	{
		sel += fullMenuPageHeight;

		if(sel >= total)
			sel = total - 1;

		editActive = 0;
	}
	else if(input_ui_pressed(machine, IPT_UI_SELECT))
	{
		if(editActive)
			editActive = 0;
		else
		{
			switch(info->fieldType)
			{
				case kType_Name:
				case kType_ExtendName:
				case kType_Comment:
				case kType_ActivationKey1:
				case kType_ActivationKey2:
				case kType_WatchSize:
				case kType_WatchSkip:
				case kType_WatchPerLine:
				case kType_WatchAddValue:
				case kType_WriteMask:
				case kType_AddMaximum:
				case kType_SubtractMinimum:
				case kType_RangeMinimum:
				case kType_RangeMaximum:
				case kType_Data:
				case kType_Address:
					osd_readkey_unicode(1);
					dirty = 1;
					editActive = 1;
					break;

				case kType_Return:
					sel = -1;
					break;
			}
		}
	}
	else if(input_ui_pressed(machine, IPT_UI_CANCEL))
	{
		if(editActive)
			editActive = 0;
		else
			sel = -1;
	}

	/********** EDIT MODE **********/
	if(editActive)
	{
		/* do edit text */
		dirty = 1;

		switch(info->fieldType)
		{
			case kType_Name:
				entry->name = DoDynamicEditTextField(entry->name);
				break;

			case kType_ExtendName:
				action->optionalName = DoDynamicEditTextField(action->optionalName);
				break;

			case kType_Comment:
				entry->comment = DoDynamicEditTextField(entry->comment);
				break;

			case kType_ActivationKey1:
			case kType_ActivationKey2:
			{
				if(input_ui_pressed(machine, IPT_UI_CANCEL))
				{
					if(info->fieldType == kType_ActivationKey1)
					{
						entry->activationKey1 = 0;
						entry->flags &= ~kCheatFlag_HasActivationKey1;
					}
					else
					{
						entry->activationKey2 = 0;
						entry->flags &= ~kCheatFlag_HasActivationKey2;
					}

					editActive = 0;
				}
				else
				{
					int code = input_code_poll_switches(FALSE);

					if(code == KEYCODE_ESC)
					{
						if(info->fieldType == kType_ActivationKey1)
						{
							entry->activationKey1 = 0;
							entry->flags &= ~kCheatFlag_HasActivationKey1;
						}
						else
						{
							entry->activationKey2 = 0;
							entry->flags &= ~kCheatFlag_HasActivationKey2;
						}

						editActive = 0;
					}
					else
					{
						if((code != INPUT_CODE_INVALID) && !input_ui_pressed(machine, IPT_UI_SELECT))
						{
							if(info->fieldType == kType_ActivationKey1)
							{
								entry->activationKey1 = code;
								entry->flags |= kCheatFlag_HasActivationKey1;
							}
							else
							{
								entry->activationKey2 = code;
								entry->flags |= kCheatFlag_HasActivationKey2;
							}

							editActive = 0;
						}
					}
				}
			}
			break;

			case kType_WatchSize:
			{
				UINT32 temp = (action->originalDataField >> 0) & 0xFF;

				temp++;
				temp = do_edit_hex_field(machine, temp) & 0xFF;
				temp--;

				action->originalDataField = (action->originalDataField & 0xFFFFFF00) | ((temp << 0) & 0x000000FF);
				action->data = action->originalDataField;
			}
			break;

			case kType_WatchSkip:
			{
				UINT32 temp = (action->originalDataField >> 8) & 0xFF;

				temp = do_edit_hex_field(machine, temp) & 0xFF;

				action->originalDataField = (action->originalDataField & 0xFFFF00FF) | ((temp << 8) & 0x0000FF00);
				action->data = action->originalDataField;
			}
			break;

			case kType_WatchPerLine:
			{
				UINT32 temp = (action->originalDataField >> 16) & 0xFF;

				temp = do_edit_hex_field(machine, temp) & 0xFF;

				action->originalDataField = (action->originalDataField & 0xFF00FFFF) | ((temp << 16) & 0x00FF0000);
				action->data = action->originalDataField;
			}
			break;

			case kType_WatchAddValue:
			{
				UINT32 temp = (action->originalDataField >> 24) & 0xFF;

				temp = DoEditHexFieldSigned(temp, 0xFFFFFF80) & 0xFF;

				action->originalDataField = (action->originalDataField & 0x00FFFFFF) | ((temp << 24) & 0xFF000000);
				action->data = action->originalDataField;
			}
			break;

			case kType_WriteMask:
				action->extendData = do_edit_hex_field(machine, action->extendData);
				action->extendData &= info->extraData;
				break;

			case kType_AddMaximum:
			case kType_SubtractMinimum:
				action->extendData = do_edit_hex_field(machine, action->extendData);
				break;

			case kType_RangeMinimum:
			{
				UINT32 temp;

				if(!TEST_FIELD(action->type, BytesUsed))
				{
					temp = (action->extendData >> 8) & 0xFF;
					temp = do_edit_hex_field(machine, temp) & 0xFF;

					action->extendData = (action->extendData & 0xFF) | ((temp << 8) & 0xFF00);
				}
				else
				{
					temp = (action->extendData >> 16) & 0xFFFF;
					temp = do_edit_hex_field(machine, temp) & 0xFFFF;

					action->extendData = (action->extendData & 0x0000FFFF) | ((temp << 16) & 0xFFFF0000);
				}
			}
			break;

			case kType_RangeMaximum:
			{
				UINT32 temp;

				if(!TEST_FIELD(action->type, BytesUsed))
				{
					temp = action->extendData & 0xFF;
					temp = do_edit_hex_field(machine, temp) & 0xFF;

					action->extendData = (action->extendData & 0xFF00) | (temp & 0x00FF);
				}
				else
				{
					temp = action->extendData & 0xFFFF;
					temp = do_edit_hex_field(machine, temp) & 0xFFFF;

					action->extendData = (action->extendData & 0xFFFF0000) | (temp & 0x0000FFFF);
				}
			}
			break;

			case kType_Data:
				action->originalDataField = do_edit_hex_field(machine, action->originalDataField);
				action->originalDataField &= info->extraData;
				action->data = action->originalDataField;
				break;

			case kType_Address:
				action->address = do_edit_hex_field(machine, action->address);
				action->address &= info->extraData;
				break;
		}
	}
	else
	{
		if(input_ui_pressed(machine, IPT_UI_SAVE_CHEAT))
		{
			SaveCheatCode(machine, entry);
		}
		else if(input_ui_pressed(machine, IPT_UI_WATCH_VALUE))
		{
			WatchCheatEntry(entry, 0);
		}
		else if(input_ui_pressed(machine, IPT_UI_ADD_CHEAT))
		{
			AddActionBefore(entry, info->sub_cheat);

			entry->actionList[info->sub_cheat].extendData = ~0;

			for(i = 0; i < entry->actionListLength; i++)
			{
				CheatAction * action = &entry->actionList[i];

				if(i)
					SET_MASK_FIELD(action->type, LinkEnable);
				else
					CLEAR_MASK_FIELD(action->type, LinkEnable);
			}
		}
		else if(input_ui_pressed(machine, IPT_UI_DELETE_CHEAT))
		{
			if(!info->sub_cheat)
			{
				/* don't delete MASTER Action due to crash */
				messageTimer = DEFAULT_MESSAGE_TIME;
				messageType = kCheatMessage_FailedToDelete;
			}
			else
				DeleteActionAt(entry, info->sub_cheat);
		}
	}

	if(dirty)
	{
		UpdateCheatInfo(entry, 0);

		entry->flags |= kCheatFlag_Dirty;
	}

	if(sel == -1)
	{
		/* NOTE : building label index table should be done when exit the edit menu */
		if(entry->flags & kCheatFlag_Select)
			BuildLabelIndexTable(entry);

		CheckCodeFormat(entry);

		editActive = 0;
		dirty = 1;
	}

	/* free astrings used by displaying the name of activation key */
	astring_free(tempString1);
	astring_free(tempString2);
#endif
	return menu->sel;
}

/*--------------------------------------------------------------
  view_cheat_menu - management for code viewer for new format
--------------------------------------------------------------*/

static int view_cheat_menu(running_machine *machine, cheat_menu_stack *menu)
{
	static const char *const kAddressSpaceNames[] = {
		"Program", "Data Space", "I/O Space", "Opcode RAM", "Mapped Memory", "Direct Memory", "EEPROM" };

	static const char *const kCodeTypeNames[] = {
		"Write", "PDWWrite", "RWrite", "IWrite", "CWrite", "VWrite", "VRWrite", "Move", "Branch", "Loop",
		"Unused 1", "Unused 2", "Unused 3", "Unused 4",
		"Popup", "Watch" };

	static const char *const kCustomCodeTypeNames[] = {
		"No Action", "Comment", "Separator", "Label Select", "Layer",
		"Unused 1", "Unused2", "Unused 3",
		/* NOTE : the following command codes should be removed when database is loaded */
		"Activation Key 1", "Activation Key 2", "Pre-enable", "Overclock", "Refresh Rate" };

	static const char *const kConditionNames[] = {
		"EQ", "NE", "LS", "LSEQ", "GR", "GREQ", "BSET", "BCLR",
		"Unused 1", "Unused 2", "Unused 3", "Unused 4",
		"PVAL", "KEY", "KEYR", "TRUE" };

	static const char *const kDataNames[] = {
		"Data", "1st Data", "Data", "Data", "Data", "Max Data", "Max Data", "Add Value", "Comparison", "Count",
		"-----", "-----", "-----", "-----",
		"Comparison", "-----" };

	static const char *const kPopupOptionNames[] = {
		"Label", "Value", "Label + Value", "Value + Label" };

	enum{
		kViewMenu_Header = 1,						// format : total index / current index
		kViewMenu_CodeLayer,						// internal value of code and layer index
		kViewMenu_Name,								// master code = entry->name, linked code = action->optionalName
			kViewMenu_Comment,
			kViewMenu_ActivationKey1,
			kViewMenu_ActivationKey2,
		// if(!CustomCode)
			kViewMenu_CodeType,							// CodeType - CodeParameter / Return
			kViewMenu_CPURegion,						// add Address Space if CPU region
			kViewMenu_Address,
			kViewMenu_Size,								// address size / index size
			kViewMenu_Data,
			kViewMenu_Extend,
			// if(ValueSelect)
				kViewMenu_ValueSelectOptions,
			// if(Popup)
				kViewMenu_PopupOptions,
			// if(CopyLink)
				kViewMenu_CopyLinkOptions,
			kViewMenu_Options,							// One-shot, Delay, Prefill, RestoreValue
		// if(CustomCode)
			//if(LayerIndex)
				kViewMenu_LayerDisplayLevel,
				kViewMenu_LayerSetLevel,
				kViewMenu_LayerSetLength,
		kViewMenu_Return,
		kViewMenu_Max };

	int				total = 0;
	int				menu_index[kViewMenu_Max] = { 0 };
	static int		page_index = 0;
	UINT8			address_length = 0;
	UINT8			address_size = 0;
	const char		** menu_item;
	const char		** menu_sub_item;
	char			* flag_buf;
	char			* header;
	char			* buf_code;
	char			* buf_type;
	char			* buf_cpu;
	char			* buf_address;
	char			* buf_size;
	char			* buf_data;
	char			* buf_extend;
	char			* buf_options;
	char			* buf_code_type_options;
	CheatEntry		* entry = &cheatList[menu->pre_sel];
	CheatAction		* action = NULL;
	astring			* activation_key_string1 = NULL;
	astring			* activation_key_string2 = NULL;

	/* first setting : forced set page as 0 when first open */
	if(menu->first_time)
	{
		page_index = 0;
		menu->first_time = 0;
	}

	action			= &entry->actionList[page_index];
	address_length	= get_address_length(action->region);
	address_size	= EXTRACT_FIELD(action->type, AddressSize);

	/* required items = (total items + return + terminator) & (strings buf * 10) */
	request_strings(kViewMenu_Max + 2, 0x0A, 32, 0);
	menu_item				= menu_strings.main_list;
	menu_sub_item			= menu_strings.sub_list;
	flag_buf				= menu_strings.flag_list;
	header					= menu_strings.main_strings[0];
	buf_code				= menu_strings.main_strings[1];
	buf_type				= menu_strings.main_strings[2];
	buf_cpu					= menu_strings.main_strings[3];
	buf_address				= menu_strings.main_strings[4];
	buf_size				= menu_strings.main_strings[5];
	buf_data				= menu_strings.main_strings[6];
	buf_extend				= menu_strings.main_strings[7];
	buf_options				= menu_strings.main_strings[8];
	buf_code_type_options	= menu_strings.main_strings[9];

	memset(flag_buf, 0, kViewMenu_Max + 2);

	/********** MENU CONSTRUCTION **********/
	/* ##### HEADER ##### */
	sprintf(header, "[%2.2d/%2.2d]", page_index + 1, entry->actionListLength);
	ADD_MENU_3_ITEMS("Page", header, kViewMenu_Header);

	/* ##### CODE / LAYER INDEX ##### */
	sprintf(buf_code, "%8.8X / %2.2X", action->type, entry->layerIndex);
	ADD_MENU_3_ITEMS("Code / Layer", buf_code, kViewMenu_CodeLayer);

	/* ##### NAME ##### */
	if(entry->name)
	{
		if(page_index)
			ADD_MENU_3_ITEMS("Name", action->optionalName, kViewMenu_Name);
		else
			ADD_MENU_3_ITEMS("Name", entry->name, kViewMenu_Name);
	}
	else
		ADD_MENU_3_ITEMS("Name", "< none >", kViewMenu_Name);

	/* ##### COMMENT ##### */
	if(page_index == 0 && entry->comment)
		ADD_MENU_3_ITEMS("Comment", entry->comment, kViewMenu_Comment);

	/* ##### ACTIVATION KEY ##### */
	if(entry->flags & kCheatFlag_Select)
	{
		if(entry->flags & kCheatFlag_HasActivationKey1)
		{
			activation_key_string1 = astring_alloc();
			ADD_MENU_3_ITEMS("Activation Key", astring_c(input_code_name(activation_key_string1, entry->activationKey1)), kViewMenu_ActivationKey1);
		}
	}
	else
	{
		if(entry->flags & kCheatFlag_HasActivationKey1)
		{
			activation_key_string1 = astring_alloc();
			ADD_MENU_3_ITEMS("Activation Key - Prev", astring_c(input_code_name(activation_key_string1, entry->activationKey1)), kViewMenu_ActivationKey1);
		}

		if(entry->flags & kCheatFlag_HasActivationKey2)
		{
			activation_key_string2 = astring_alloc();
			ADD_MENU_3_ITEMS("Activation Key - Next", astring_c(input_code_name(activation_key_string2, entry->activationKey2)), kViewMenu_ActivationKey2);
		}
	}

	/* ##### CODE TYPE ##### */
	if(action->flags & kActionFlag_Custom)
	{
		ADD_MENU_3_ITEMS("Custom Type", kCustomCodeTypeNames[EXTRACT_FIELD(action->type, CodeType)], kViewMenu_CodeType);
	}
	else
	{
		char * type_strings = buf_type;

		type_strings += sprintf(type_strings, "%s", kCodeTypeNames[EXTRACT_FIELD(action->type, CodeType)]);

		if(action->flags & kActionFlag_CheckCondition)
			type_strings += sprintf(type_strings, "-%s", kConditionNames[EXTRACT_FIELD(action->type, CodeParameter)]);

		if(TEST_FIELD(action->type, Return))
			type_strings += sprintf(type_strings, "/Ret");

		ADD_MENU_3_ITEMS("Type", buf_type, kViewMenu_CodeType);
	}

	if((action->flags & kActionFlag_Custom) == 0)
	{
		/* ##### CPU/REGION ##### */
		if(TEST_FIELD(action->region, RegionFlag))
		{
			ADD_MENU_3_ITEMS("Region", kRegionNames[EXTRACT_FIELD(action->region, RegionIndex)], kViewMenu_CPURegion);
		}
		else
		{
			sprintf(buf_cpu, "%s (%s)", kRegionNames[EXTRACT_FIELD(action->region, CPUIndex) + 1], kAddressSpaceNames[EXTRACT_FIELD(action->region, AddressSpace)]);
			ADD_MENU_3_ITEMS("CPU", buf_cpu, kViewMenu_CPURegion);
		}

		/* ##### ADDRESS ##### */
		switch(EXTRACT_FIELD(action->type, AddressRead))
		{
			case kReadFrom_Address:
				sprintf(buf_address, "%*.*X", address_length, address_length, action->originalAddress);
				break;

			case kReadFrom_Index:
				sprintf(buf_address, "(V%s)", kNumbersTable[action->originalAddress]);
				break;

			case kReadFrom_Variable:
				sprintf(buf_address, "V%s", kNumbersTable[action->originalAddress]);
		}

		ADD_MENU_3_ITEMS("Address", buf_address, kViewMenu_Address);

		/* ##### SIZE ##### */
		if(action->flags & kActionFlag_PDWWrite)
		{
			sprintf(buf_size, "%s / %s", kByteSizeStringList[address_size], kByteSizeStringList[EXTRACT_FIELD(action->type, IndexSize)]);
			ADD_MENU_3_ITEMS("Size 1st / 2nd", buf_size, kViewMenu_Size);
		}
		else if(action->flags & kActionFlag_IndexAddress)
		{
			if(action->extendData != ~0)
			{
				sprintf(buf_size, "%s / %s", kByteSizeStringList[address_size], kByteSizeStringList[EXTRACT_FIELD(action->type, IndexSize)]);
			}
			else
				sprintf(buf_size, "%s / < None >", kByteSizeStringList[address_size]);

			ADD_MENU_3_ITEMS("Size Adr / Idx", buf_size, kViewMenu_Size);
		}
		else
		{
			sprintf(buf_size, "%s", kByteSizeStringList[address_size]);
			ADD_MENU_3_ITEMS("Size", buf_size, kViewMenu_Size);
		}

		/* ##### DATA ##### */
		if(EXTRACT_FIELD(action->type, CodeType) != kCodeType_Watch)
		{
			if(TEST_FIELD(action->type, DataRead))
			{
				sprintf(buf_data, "V%s", kNumbersTable[action->originalDataField]);
			}
			else
			{
				sprintf(buf_data, "%*.*X", kCheatSizeDigitsTable[address_size], kCheatSizeDigitsTable[address_size], action->originalDataField);
			}

			if(EXTRACT_FIELD(action->type, Link) != kLink_CopyLink)
				ADD_MENU_3_ITEMS(kDataNames[EXTRACT_FIELD(action->type, CodeType)], buf_data, kViewMenu_Data);
			else
				ADD_MENU_3_ITEMS("Add Value", buf_data, kViewMenu_Data);
		}

		/* ##### EXTEND DATA ##### */
		if(action->flags & kActionFlag_IndexAddress)
		{
			if(action->extendData != ~0)
				sprintf(buf_extend, "%*.8X", address_length, action->extendData);
			else
				sprintf(buf_extend, "< none >");

			ADD_MENU_3_ITEMS("Index", buf_extend, kViewMenu_Extend);
		}
		else if(action->flags & kActionFlag_PDWWrite)
		{
			sprintf(buf_extend, "%*.*X",
				kCheatSizeDigitsTable[EXTRACT_FIELD(action->type, IndexSize)],
				kCheatSizeDigitsTable[EXTRACT_FIELD(action->type, IndexSize)],
				action->extendData);
			ADD_MENU_3_ITEMS("2nd Data", buf_extend, kViewMenu_Extend);
		}
		else if(action->flags & kActionFlag_Repeat)
		{
			sprintf(buf_extend, "%4.4X / %4.4X",
						EXTRACT_FIELD(action->extendData, LSB16),
						EXTRACT_FIELD(action->extendData, MSB16));
			ADD_MENU_3_ITEMS("Count / Skip", buf_extend, kViewMenu_Extend);
		}
		else
		{
			switch(EXTRACT_FIELD(action->type, CodeType))
			{
				case kCodeType_Write:
				case kCodeType_IWrite:
				case kCodeType_VWrite:
					sprintf(buf_extend, "%*.*X", address_size, address_size, action->extendData);
					ADD_MENU_3_ITEMS("Mask", buf_extend, kViewMenu_Extend);
					break;

				case kCodeType_CWrite:
					sprintf(buf_extend, "%*.*X", address_size, address_size, action->extendData);
					ADD_MENU_3_ITEMS("Comparison", buf_extend, kViewMenu_Extend);
					break;

				case kCodeType_Branch:
					sprintf(buf_extend, "%2.2d", action->extendData + 1);
					ADD_MENU_3_ITEMS("Jump Index", buf_extend, kViewMenu_Extend);
					break;

				default:
					ADD_MENU_3_ITEMS("UNKNOWN", "?????", kViewMenu_Extend);
			}
		}

		/* ##### OPTIONS ##### */
		sprintf(buf_options, "%s %s %s %s",
					TEST_FIELD(action->type, OneShot) ? "O" : "-",
					TEST_FIELD(action->type, DelayEnable) ? "D" : "-",
					TEST_FIELD(action->type, PrefillEnable) ? "P" : "-",
					TEST_FIELD(action->type, RestoreValue) ? "R" : "-");
		ADD_MENU_3_ITEMS("Options", buf_options, kViewMenu_Options);

		/* ##### CODE TYPE OPTIONS ##### */
		if(	EXTRACT_FIELD(action->type, CodeType) == kCodeType_VWrite ||
			EXTRACT_FIELD(action->type, CodeType) == kCodeType_VRWrite)
		{
			sprintf(buf_code_type_options, "%s %s %s %s",
						TEST_FIELD(action->type, ValueSelectMinimumDisplay) ? "D" : "-",
						TEST_FIELD(action->type, ValueSelectMinimum) ? "M" : "-",
						TEST_FIELD(action->type, ValueSelectBCD) ? "B" : "-",
						TEST_FIELD(action->type, ValueSelectNegative) ? "N" : "-");
			ADD_MENU_3_ITEMS("Value Select Options", buf_code_type_options, kViewMenu_ValueSelectOptions);
		}
		else if(EXTRACT_FIELD(action->type, CodeType) == kCodeType_Popup)
		{
			ADD_MENU_3_ITEMS("Popup Options", kPopupOptionNames[EXTRACT_FIELD(action->type, PopupParameter)], kViewMenu_PopupOptions);
		}
		else if(EXTRACT_FIELD(action->type, Link) == kLink_CopyLink)
		{
			sprintf(buf_code_type_options, "%s %s",
						TEST_FIELD(action->type, LinkValueSelectBCD) ? "B" : "-",
						TEST_FIELD(action->type, LinkValueSelectNegative) ? "N" : "-");
			ADD_MENU_3_ITEMS("Copy Link Options", buf_code_type_options, kViewMenu_CopyLinkOptions);
		}
	}
	else
	{
		if(EXTRACT_FIELD(action->type, CodeType) == kCustomCode_Layer)
		{
			/* ##### LAYER ##### */
			sprintf(buf_address, "%2.2X", action->originalAddress);
			ADD_MENU_3_ITEMS("Display Level", buf_address, kViewMenu_LayerDisplayLevel);

			sprintf(buf_data, "%2.2X", action->originalDataField);
			ADD_MENU_3_ITEMS("Set Level", buf_data, kViewMenu_LayerSetLevel);

			sprintf(buf_extend, "%2.2X", action->extendData);
			ADD_MENU_3_ITEMS("Set Length", buf_extend, kViewMenu_LayerSetLength);
		}
	}

	/* ##### RETURN ##### */
	ADD_MENU_3_ITEMS("Return to Prior Menu", NULL, kViewMenu_Return);

	/* ##### TERMINATE ARRAY ##### */
	TERMINATE_MENU_2_ITEMS;

	/* adjust current cursor position */
	ADJUST_CURSOR(menu->sel, total)

	/* draw it */
	old_style_menu(menu_item, menu_sub_item, flag_buf, menu->sel, 0);

	/********** KEY HANDLING **********/
	if(ui_pressed_repeat_throttle(machine, IPT_UI_UP, kVerticalKeyRepeatRate))
	{
		CURSOR_TO_PREVIOUS(menu->sel, total)
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_DOWN, kVerticalKeyRepeatRate))
	{
		CURSOR_TO_NEXT(menu->sel, total)
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_LEFT, kHorizontalSlowKeyRepeatRate))
	{
		if(--page_index < 0)
			page_index = entry->actionListLength - 1;
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_RIGHT, kHorizontalSlowKeyRepeatRate))
	{
		if(++page_index > entry->actionListLength - 1)
			page_index = 0;
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_PAGE_UP, kVerticalKeyRepeatRate))
	{
		CURSOR_PAGE_UP(menu->sel)
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_PAGE_DOWN, kVerticalKeyRepeatRate))
	{
		CURSOR_PAGE_DOWN(menu->sel, total)
	}
	else if(input_ui_pressed(machine, IPT_UI_SELECT) || input_ui_pressed(machine, IPT_UI_CANCEL))
	{
		menu->sel = -1;
	}
	else if(input_ui_pressed(machine, IPT_UI_SAVE_CHEAT))
	{
		SaveCheatCode(machine, entry);
	}

	if(activation_key_string1)
		astring_free(activation_key_string1);
	if(activation_key_string2)
		astring_free(activation_key_string2);

	return menu->sel + 1;
}

/*---------------------------------------------------------
  analyse_cheat_menu - management for code analizer menu
---------------------------------------------------------*/

static int analyse_cheat_menu(running_machine *machine, cheat_menu_stack *menu)
{
	static const char *const kAnalyseItemName[] = {
		"Invalid Location Type",
		"Invalid Operation",
		"Invalid Code Type",
		"Invalid Condition",
		"Invalid Code Option",
		"Relative Address conflicts other operations",
		"Maximum range is smaller than minimum",
		"Missing Restore Previous Value flag",
		"Missing label link code",
		"Both Value and Label Select are defined",
		"Copy Link is defined though no value-select code",
		"Invalid Data Field",
		"Invalid Extend Data Field",
		"Don't set Variable in this type",
		"Value Select can't select any value",
		"RWrite doesn't have counter",
		"Invalid Address Read",
		"Invalid Variable in address field",
		"Invalid Variable in extend data field",
		"CPU/Region is out of range",
		"Invalid CPU/Region number",
		"Invalid Address Space",
		"An address is out of range",
		"Invalid Custom Code" };

	int				i, j;
	int				total			= 0;
	UINT8			isEdit			= 0;
	const char		** menuItem;
	CheatEntry		* entry			= &cheatList[menu->pre_sel];

	/* first setting : NONE */
	if(menu->first_time)
		menu->first_time = 0;

	/* required items = ((items + header + 2 separators) * total_actions) + edit + return + terminator) */
	request_strings(((kErrorFlag_Max + 3) * entry->actionListLength) + 3, 0, 0, 0);
	menuItem = menu_strings.main_list;

	/********** MENU CONSTRUCTION **********/
	for(i = 0; i < entry->actionListLength; i++)
	{
		CheatAction * action = &entry->actionList[i];

		UINT32 flags = AnalyseCodeFormat(entry, action);

		menuItem[total++] = action->optionalName ? action->optionalName : "(Null)";
		menuItem[total++] = MENU_SEPARATOR_ITEM;

		if(flags)
		{
			for(j = 0; j < kErrorFlag_Max; j++)
			{
				if((flags >> j) & 1)
					menuItem[total++] = kAnalyseItemName[j];
			}
		}
		else
			menuItem[total++] = "No Problem";

		menuItem[total++] = MENU_SEPARATOR_ITEM;
	}

	menuItem[total++] = "Edit This Entry";
	menuItem[total++] = "Return to Prior Menu";
	menuItem[total] = NULL;

	/* adjust cursor position */
	ADJUST_CURSOR(menu->sel, total)

	old_style_menu(menuItem, NULL, NULL, menu->sel, 0);

	if(ui_pressed_repeat_throttle(machine, IPT_UI_UP, kVerticalKeyRepeatRate))
	{
		CURSOR_TO_PREVIOUS(menu->sel, total)
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_DOWN, kVerticalKeyRepeatRate))
	{
		CURSOR_TO_NEXT(menu->sel, total)
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_PAGE_UP, kVerticalKeyRepeatRate))
	{
		CURSOR_PAGE_UP(menu->sel)
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_PAGE_DOWN, kVerticalKeyRepeatRate))
	{
		CURSOR_PAGE_DOWN(menu->sel, total)
	}
	else if(input_ui_pressed(machine, IPT_UI_EDIT_CHEAT))
	{
		isEdit = 1;
	}
	else if(input_ui_pressed(machine, IPT_UI_SELECT))
	{
		if(menu->sel == total - 2)
			isEdit = 1;
		else
			menu->sel = -1;
	}
	else if(input_ui_pressed(machine, IPT_UI_CANCEL))
	{
		menu->sel = -1;
	}

	if(isEdit)
	{
		if(entry->flags & kCheatFlag_OldFormat)
			cheat_menu_stack_push(edit_cheat_menu, menu->return_handler, menu->pre_sel);
		else
			cheat_menu_stack_push(view_cheat_menu, menu->return_handler, menu->pre_sel);
	}

	return menu->sel + 1;
}

/*-----------------------------------------------------------
  search_minimum_menu - management for minimum search menu
                        limited size, operand, comparison
-----------------------------------------------------------*/

static int search_minimum_menu(running_machine *machine, cheat_menu_stack *menu)
{
	enum{
		kMenu_CPU = 0,
		kMenu_Memory,
		kMenu_Divider1,
		kMenu_Value,
		kMenu_Timer,
		kMenu_Greater,
		kMenu_Less,
		kMenu_Equal,
		kMenu_NotEqual,
		kMenu_Divider2,
		kMenu_Result,
		kMenu_RestoreSearch,
		kMenu_Return,
		kMenu_Max };

	INT8			i;
	UINT8			total			= 0;
	UINT8			doSearch		= 0;
	static UINT8	doneSaveMemory	= 0;
	const char		* menuItem[kMenu_Max + 1]		= { 0 };
	const char		* menuSubItem[kMenu_Max + 1]	= { 0 };
	char			flagBuf[kMenu_Max + 1]			= { 0 };
	char			cpuBuf[4];
	char			valueBuf[32];		// "FFFFFFF[F] (4294967295)"    23 chars
	char			timerBuf[32];		// "-FFFFFFF[F] (4294967295)"   24 chars
	char			numResultsBuf[16];
	char			* stringsBuf;
	search_info		*search = get_current_search();

	/* first setting : NONE */
	if(menu->first_time)
		menu->first_time = 0;

	/* NOTE : in this mode, search bytes is always fixed with 1 byte right now */
	search->old_options.value &= kSearchByteMaskTable[search->bytes];
	search->old_options.delta &= kSearchByteMaskTable[search->bytes];

	/********** MENU CONSTRUCTION **********/
	/* ##### CPU ##### */
	menuItem[total] = "CPU";
	sprintf(cpuBuf, "%d", search->target_idx);
	menuSubItem[total++] = cpuBuf;

	/* ##### MEMORY ##### */
	if(doneSaveMemory)
		menuItem[total] = "Initialize Memory";
	else
		menuItem[total] = "Save Memory";
	menuSubItem[total++] = NULL;

	/* ##### DIVIDER ##### */
	menuItem[total] = MENU_SEPARATOR_ITEM;
	menuSubItem[total++] = NULL;

	/* ##### VALUE ##### */
	menuItem[total] = "Value";

	if(editActive && menu->sel == kMenu_Value)
	{
		stringsBuf = valueBuf;

		for(i = 1; i >= 0; i--)
		{
			if(i == editCursor)
				stringsBuf += sprintf(stringsBuf, "[%X]", (search->old_options.value & kIncrementMaskTable[i]) / kIncrementValueTable[i]);
			else
				stringsBuf += sprintf(stringsBuf, "%X", (search->old_options.value & kIncrementMaskTable[i]) / kIncrementValueTable[i]);
		}

		stringsBuf += sprintf(stringsBuf, " (%d)", search->old_options.value);
	}
	else
		sprintf(valueBuf, "%.*X (%d)",
			kSearchByteDigitsTable[search->bytes], search->old_options.value, search->old_options.value);

	menuSubItem[total++] = valueBuf;

	/* ##### TIMER ##### */
	menuItem[total] = "Timer";

	if(!(search->old_options.delta & kSearchByteSignBitTable[search->bytes]))
	{
		/* PLUS */
		if(editActive && menu->sel == kMenu_Timer)
		{
			stringsBuf = timerBuf;

			for(i = 2; i >= 0; i--)
			{
				if(i == editCursor)
				{
					if(i == 2)
						stringsBuf += sprintf(stringsBuf, "[+]");
					else
						stringsBuf += sprintf(stringsBuf, "[%X]", (search->old_options.delta & kIncrementMaskTable[i]) / kIncrementValueTable[i]);
				}
				else
				{
					if(i == 2)
						stringsBuf += sprintf(stringsBuf, "+");
					else
						stringsBuf += sprintf(stringsBuf, "%X", (search->old_options.delta & kIncrementMaskTable[i]) / kIncrementValueTable[i]);
				}
			}

			stringsBuf += sprintf(stringsBuf, " (+%d)", search->old_options.delta);
		}
		else
			sprintf(timerBuf, "+%.2X (+%d)", search->old_options.delta, search->old_options.delta);
	}
	else
	{
		UINT32 deltaDisp = (~search->old_options.delta + 1) & 0x7F;

		/* MINUS */
		if(editActive && menu->sel == kMenu_Timer)
		{
			stringsBuf = timerBuf;

			for(i = 2; i >= 0; i--)
			{
				if(i == editCursor)
				{
					if(i == 2)
						stringsBuf += sprintf(stringsBuf, "[-]");
					else
						stringsBuf += sprintf(stringsBuf, "[%X]", (deltaDisp & kIncrementMaskTable[i]) / kIncrementValueTable[i]);
				}
				else
				{
					if(i == 2)
						stringsBuf += sprintf(stringsBuf, "-");
					else
						stringsBuf += sprintf(stringsBuf, "%X", (deltaDisp & kIncrementMaskTable[i]) / kIncrementValueTable[i]);
				}
			}

#ifdef MAME_DEBUG
			stringsBuf += sprintf(stringsBuf, " = %.2X (-%d)", search->old_options.delta, deltaDisp);
#else
			stringsBuf += sprintf(stringsBuf, " (-%d)", deltaDisp);
#endif
		}
		else

#ifdef MAME_DEBUG
		sprintf(timerBuf, "-%.2X = %.2X (-%d)", deltaDisp, search->old_options.delta, deltaDisp);
#else
		sprintf(timerBuf, "-%.2X (-%d)", deltaDisp, deltaDisp);
#endif
	}
	menuSubItem[total++] = timerBuf;

	/* ##### GREATER ##### */
	menuItem[total] = "Greater";
	menuSubItem[total++] = "A > B";

	/* ##### LESS ##### */
	menuItem[total] = "Less";
	menuSubItem[total++] = "A < B";

	/* ##### EQUAL ##### */
	menuItem[total] = "Equal";
	menuSubItem[total++] = "A = B";

	/* ##### NOT EQUAL ##### */
	menuItem[total] = "Not Equal";
	menuSubItem[total++] = "A != B";

	/* ##### DIVIDER ##### */
	menuItem[total] = MENU_SEPARATOR_ITEM;
	menuSubItem[total++] = NULL;

	/* ##### RESULT ##### */
	menuItem[total] = "Results";
	if(search->num_results)
	{
		sprintf(numResultsBuf, "%d", search->num_results);
	}
	else
	{
		if(!doneSaveMemory)
			sprintf(numResultsBuf, "No Memory Save");
		else
			sprintf(numResultsBuf, "No Result");
	}
	menuSubItem[total++] = numResultsBuf;

	/* ##### RESULT RESTORE ##### */
	menuItem[total] = "Restore Previous Results";
	menuSubItem[total++] = NULL;

	/* ##### RETURN ##### */
	menuItem[total] = "Return to Prior Menu";
	menuSubItem[total++] = NULL;

	/* ##### TERMINATE ARRAY ##### */
	menuItem[total] = NULL;
	menuSubItem[total] = NULL;

	/* adjust current cursor position */
	if(menu->sel < 0 || menu->sel >= total)
		menu->sel = kMenu_Memory;
	if(!doneSaveMemory)
	{
		if(menu->sel >= kMenu_Timer && menu->sel <= kMenu_NotEqual)
			menu->sel = kMenu_Memory;
	}

	if(editActive)
		flagBuf[menu->sel] = 1;

	/* draw it */
	old_style_menu(menuItem, menuSubItem, flagBuf, menu->sel, 0);

	/********** KEY HANDLING **********/
	if(ui_pressed_repeat_throttle(machine, IPT_UI_UP, kVerticalKeyRepeatRate))
	{
		if(editActive)
		{
			switch(menu->sel)
			{
				case kMenu_Value:
					search->old_options.value = DoIncrementHexField(search->old_options.value, editCursor);
					break;

				case kMenu_Timer:
					switch(editCursor)
					{
						default:
							search->old_options.delta = DoIncrementHexFieldSigned(search->old_options.delta, editCursor, search->bytes);
							break;

						case 2:
							if(!search->old_options.delta || search->old_options.delta == 0x80)
								search->old_options.delta ^= 0x80;
							else
								search->old_options.delta = ~search->old_options.delta + 1;
							break;
					}
					break;
			}
		}
		else
		{
			CURSOR_TO_PREVIOUS(menu->sel, total)

			if(menu->sel == kMenu_Divider1 || menu->sel == kMenu_Divider2)
			{
				if(--menu->sel == kMenu_NotEqual && !doneSaveMemory)
					menu->sel = kMenu_Value;
			}
		}
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_DOWN, kVerticalKeyRepeatRate))
	{
		if(editActive)
		{
			switch(menu->sel)
			{
				case kMenu_Value:
					search->old_options.value = DoDecrementHexField(search->old_options.value, editCursor);
					break;

				case kMenu_Timer:
					switch(editCursor)
					{
						default:
							search->old_options.delta = DoDecrementHexFieldSigned(search->old_options.delta, editCursor, search->bytes);
							break;

						case 2:
							if(!search->old_options.delta || search->old_options.delta == 0x80)
								search->old_options.delta ^= 0x80;
							else
								search->old_options.delta = ~search->old_options.delta + 1;
							break;
					}
					break;
			}
		}
		else
		{
			CURSOR_TO_NEXT(menu->sel, total)

			if(menu->sel == kMenu_Divider1 || menu->sel == kMenu_Divider2)
				menu->sel++;
			if(!doneSaveMemory && menu->sel == kMenu_Timer)
				menu->sel = kMenu_Result;
		}
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_LEFT, kHorizontalFastKeyRepeatRate))
	{
		if(editActive)
		{
			switch(menu->sel)
			{
				case kMenu_Value:
					editCursor ^= 1;
					break;

				case kMenu_Timer:
					if(++editCursor > 2)
						editCursor = 0;
					break;
			}
		}
		else
		{
			switch(menu->sel)
			{
				case kMenu_CPU:
					if(search->target_idx > 0)
					{
						search->target_idx--;

						build_search_regions(machine, search);
						allocate_search_regions(search);

						doneSaveMemory = 0;
					}
					break;

				case kMenu_Value:
					search->old_options.value = (search->old_options.value - 1) & 0xFF;
					break;

				case kMenu_Timer:
					search->old_options.delta = (search->old_options.delta - 1) & 0xFF;
					break;

				default:
					menu->sel = kMenu_Memory;
			}
		}
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_RIGHT, kHorizontalFastKeyRepeatRate))
	{
		if(editActive)
		{
			switch(menu->sel)
			{
				case kMenu_Value:
					editCursor ^= 1;
					break;

				case kMenu_Timer:
					if(--editCursor < 0)
						editCursor = 2;
					break;
			}
		}
		else
		{
			switch(menu->sel)
			{
				case kMenu_CPU:
					if(search->target_idx < cpu_gettotalcpu() - 1)
					{
						search->target_idx++;

						build_search_regions(machine, search);
						allocate_search_regions(search);

						doneSaveMemory = 0;
					}
					break;

				case kMenu_Value:
					search->old_options.value = (search->old_options.value + 1) & 0xFF;
					break;

				case kMenu_Timer:
					search->old_options.delta = (search->old_options.delta + 1) & 0xFF;
					break;

				default:
					menu->sel = kMenu_Result;
			}
		}
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_PAGE_UP, kVerticalKeyRepeatRate))
	{
		CURSOR_PAGE_UP(menu->sel)
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_PAGE_DOWN, kVerticalKeyRepeatRate))
	{
		CURSOR_PAGE_DOWN(menu->sel, total)
	}
	else if(input_ui_pressed(machine, IPT_UI_SELECT))
	{
		if(editActive)
		{
			switch(menu->sel)
			{
				case kMenu_Value:
					search->bytes =			kSearchSize_8Bit;
					search->lhs =			kSearchOperand_Current;
					search->rhs =			kSearchOperand_Value;
					search->comparison =	kSearchComparison_EqualTo;
					search->value =			search->old_options.value;
					doSearch = 1;
					break;

				case kMenu_Timer:
					search->bytes =			kSearchSize_8Bit;
					search->lhs =			kSearchOperand_Current;
					search->rhs =			kSearchOperand_Previous;
					search->comparison =	kSearchComparison_IncreasedBy;
					search->value =			search->old_options.delta;
					doSearch = 1;
					break;
			}
			editActive = 0;
		}
		else
		{
			switch(menu->sel)
			{
				case kMenu_CPU:
					cheat_menu_stack_push(select_search_region_menu, menu->handler, menu->sel);
					break;

				case kMenu_Memory:
					doneSaveMemory = 0;
					doSearch = 1;
					break;

				case kMenu_Value:
					search->bytes =			kSearchSize_8Bit;
					search->lhs =			kSearchOperand_Current;
					search->rhs =			kSearchOperand_Value;
					search->comparison =	kSearchComparison_EqualTo;
					search->value =			search->old_options.value;
					doSearch = 1;
					break;

				case kMenu_Timer:
					search->bytes =			kSearchSize_8Bit;
					search->lhs =			kSearchOperand_Current;
					search->rhs =			kSearchOperand_Previous;
					search->comparison =	kSearchComparison_IncreasedBy;
					search->value =			search->old_options.delta;
					doSearch = 1;
					break;

				case kMenu_Equal:
					search->bytes =			kSearchSize_8Bit;
					search->lhs =			kSearchOperand_Current;
					search->rhs =			kSearchOperand_Previous;
					search->comparison =	kSearchComparison_EqualTo;
					doSearch = 1;
					break;

				case kMenu_NotEqual:
					search->bytes =			kSearchSize_8Bit;
					search->lhs =			kSearchOperand_Current;
					search->rhs =			kSearchOperand_Previous;
					search->comparison =	kSearchComparison_NotEqual;
					doSearch = 1;
					break;

				case kMenu_Less:
					search->bytes =			kSearchSize_8Bit;
					search->lhs =			kSearchOperand_Current;
					search->rhs =			kSearchOperand_Previous;
					search->comparison =	kSearchComparison_LessThan;
					doSearch = 1;
					break;

				case kMenu_Greater:
					search->bytes =			kSearchSize_8Bit;
					search->lhs =			kSearchOperand_Current;
					search->rhs =			kSearchOperand_Previous;
					search->comparison =	kSearchComparison_GreaterThan;
					doSearch = 1;
					break;

				case kMenu_Result:
					if(search->region_list_length)
						cheat_menu_stack_push(view_search_result_menu, menu->handler, menu->sel);
					else
					{
						/* if no search region (eg, sms.c in HazeMD), don't open result viewer to avoid the crash */
						messageTimer =	DEFAULT_MESSAGE_TIME;
						messageType =	kCheatMessage_NoSearchRegion;
					}
					break;

				case kMenu_RestoreSearch:
					restore_search_backup(search);
					break;

				case kMenu_Return:
					menu->sel = -1;
					break;
			}
		}
	}
	else if(input_ui_pressed(machine, IPT_UI_EDIT_CHEAT))
	{
		if(editActive)
			editActive = 0;
		else
		{
			if(menu->sel == kMenu_Value || menu->sel == kMenu_Timer)
			{
				editActive = 1;
				editCursor = 0;
			}
		}
	}
	else if(input_ui_pressed(machine, IPT_UI_RELOAD_CHEAT))
	{
		restore_search_backup(search);
	}
	else if(input_ui_pressed(machine, IPT_UI_CANCEL))
	{
		if(editActive)
			editActive = 0;
		else
			menu->sel = -1;
	}

	/********** SEARCH **********/
	if(doSearch)
	{
		if(!doneSaveMemory)
			init_new_search(search);

		if(doneSaveMemory || menu->sel == kMenu_Value)
		{
			backup_search(search);
			do_search(search);
		}

		update_search(search);

		if(doneSaveMemory || menu->sel == kMenu_Value)
			messageType = kCheatMessage_CheatFound;
		else
			messageType = kCheatMessage_InitializeMemory;

		messageTimer = DEFAULT_MESSAGE_TIME;

		doneSaveMemory = 1;

		if(search->num_results == 1)
		{
			AddCheatFromFirstResult(search);

			messageType = kCheatMessage_1CheatFound;
		}
	}

	/********** EDIT **********/
	if(editActive)
	{
		switch(menu->sel)
		{
			case kMenu_Value:
				search->old_options.value = do_edit_hex_field(machine, search->old_options.value);
				search->old_options.value &= 0xFF;
				break;

			case kMenu_Timer:
				search->old_options.delta = do_edit_hex_field(machine, search->old_options.delta);
				search->old_options.delta &= 0xFF;
				break;
		}
	}

	return menu->sel + 1;
}

/*-------------------------------------------------------------
  search_standard_menu - management for standard search menu
                         fixed bit and operand
-------------------------------------------------------------*/

static int search_standard_menu(running_machine *machine, cheat_menu_stack *menu)
{
	static const char *const kComparisonNameTable[] = {
		"Equal",
		"Not Equal",
		"Less",
		"Less or Equal",
		"Greater",
		"Greater or Equal" };

	static const char *const kIncrementNameTable[] = {
		"Increment",
		"Decrement" };

	static const int kComparisonConversionTable[] = {
		kSearchComparison_EqualTo,
		kSearchComparison_NotEqual,
		kSearchComparison_LessThan,
		kSearchComparison_LessThanOrEqualTo,
		kSearchComparison_GreaterThan,
		kSearchComparison_GreaterThanOrEqualTo };

	enum{
		kMenu_Comparison,
		kMenu_Value,
		kMenu_NearTo,
		kMenu_Increment,
		kMenu_Bit,
		kMenu_Separator,
		kMenu_CPU,
		kMenu_Memory,
		kMenu_ViewResult,
		kMenu_RestoreResult,
		kMenu_Return,

		kMenu_Max };

	enum{
		kComparison_Equal = 0,
		kComparison_NotEqual,
		kComparison_Less,
		kComparison_LessOrEqual,
		kComparison_Greater,
		kComparison_GreaterOrEqual,
		kComparison_Max = kComparison_GreaterOrEqual };

	INT8			total			= 0;
	INT8			doSearch		= 0;
	static INT8		doneSaveMemory	= 0;
	const char		* menuItem[kMenu_Max + 1]		= { 0 };
	const char		* menuSubItem[kMenu_Max + 1]	= { 0 };
	char			valueBuf[32];
	char			incrementBuf[16];
	search_info		*search = get_current_search();

	/* first setting : adjust search items */
	if(menu->first_time)
	{
		search->lhs = kSearchOperand_Current;

		if(!search->old_options.delta)
			search->old_options.delta = 1;

		menu->first_time = 0;
	}

	/********** MENU CONSTRUCTION **********/
	/* ##### COMPARISON ##### */
	menuItem[total] = "Comparison";
	if(menu->sel == kMenu_Comparison || menu->sel == kMenu_Value)
		menuSubItem[total++] = kComparisonNameTable[search->old_options.comparison];
	else
		menuSubItem[total++] = " ";

	/* ##### VALUE ##### */
	menuItem[total] = "Value";
	if(menu->sel == kMenu_Value)
	{
		if(editActive)
		{
			INT8	i;
			char	* stringsBuf = valueBuf;

			for(i = 1; i >= 0; i--)
			{
				if(i == editCursor)
					stringsBuf += sprintf(stringsBuf, "[%X]", (search->old_options.value & kIncrementMaskTable[i]) / kIncrementValueTable[i]);
				else
					stringsBuf += sprintf(stringsBuf, "%X", (search->old_options.value & kIncrementMaskTable[i]) / kIncrementValueTable[i]);
			}
			stringsBuf += sprintf(stringsBuf, " (%d)", search->old_options.value);
		}
		else
			sprintf(valueBuf,	"%2.2X (%2.2d)", search->old_options.value, search->old_options.value);

		menuSubItem[total++] = valueBuf;
	}
	else
		menuSubItem[total++] = " ";

	/* ##### VALUE - NEAR TO ##### */
	menuItem[total] = "Near To";
	if(menu->sel == kMenu_NearTo)
	{
		if(editActive)
		{
			INT8	i;
			char	* stringsBuf = valueBuf;

			for(i = 1; i >=0; i--)
			{
				if(i == editCursor)
					stringsBuf += sprintf(stringsBuf, "[%X]", (search->old_options.value & kIncrementMaskTable[i]) / kIncrementValueTable[i]);
				else
					stringsBuf += sprintf(stringsBuf, "%X", (search->old_options.value & kIncrementMaskTable[i]) / kIncrementValueTable[i]);
			}
			stringsBuf += sprintf(stringsBuf,	" or %2.2X (%2.2d or %2.2d)",
												(search->old_options.value - 1) & kSearchByteMaskTable[search->bytes],
												search->old_options.value,
												search->old_options.value - 1);
		}
		else
			sprintf(valueBuf,	"%2.2X or %2.2X (%2.2d or %2.2d)",
								search->old_options.value, (search->old_options.value - 1) & kSearchByteMaskTable[search->bytes],
								search->old_options.value, search->old_options.value - 1);
		menuSubItem[total++] = valueBuf;
	}
	else
		menuSubItem[total++] = " ";

	/* ##### INCREMENT/DECREMENT ##### */
	menuItem[total] = kIncrementNameTable[search->old_options.sign];
	if(menu->sel == kMenu_Increment)
	{
		if(editActive)
		{
			INT8	i;
			char	* stringsBuf = incrementBuf;

			for(i = 2; i >=0; i--)
			{
				if(i == editCursor)
				{
					if(i == 2)
						stringsBuf += sprintf(stringsBuf, "[%s]", search->old_options.sign ? "-" : "+");
					else
						stringsBuf += sprintf(stringsBuf, "[%X]", (search->old_options.delta & kIncrementMaskTable[i]) / kIncrementValueTable[i]);
				}
				else
				{
					if(i == 2)
						stringsBuf += sprintf(stringsBuf, "%s", search->old_options.sign ? "-" : "+");
					else
						stringsBuf += sprintf(stringsBuf, "%X", (search->old_options.delta & kIncrementMaskTable[i]) / kIncrementValueTable[i]);
				}
			}
			stringsBuf += sprintf(stringsBuf, " (%s%2.2d)", search->old_options.sign ? "-" : "+", search->old_options.delta);
		}
		else
		{
			sprintf(incrementBuf,	"%s%2.2X (%s%2.2d)",
									search->old_options.sign ? "-" : "+",
									search->old_options.delta,
									search->old_options.sign ? "-" : "+",
									search->old_options.delta);
		}
		menuSubItem[total++] = incrementBuf;
	}
	else
		menuSubItem[total++] = " ";

	/* ##### BIT ##### */
	menuItem[total] = "Bit";
	if(menu->sel == kMenu_Bit)
		menuSubItem[total++] = kComparisonNameTable[search->old_options.comparison];
	else
		menuSubItem[total++] = " ";

	/* ##### SEPARATOR ##### */
	menuItem[total] = MENU_SEPARATOR_ITEM;
	menuSubItem[total++] = NULL;

	/* ##### CPU ##### */
	menuItem[total] = "CPU";
	menuSubItem[total++] = kNumbersTable[search->target_idx];

	/* ##### MEMORY ##### */
	if(doneSaveMemory)
		menuItem[total] = "Initialize Memory";
	else
		menuItem[total] = "Save Memory";
	menuSubItem[total++] = NULL;

	/* ##### RESULT VIEWER ##### */
	menuItem[total] = "View Last Results";
	menuSubItem[total++] = NULL;

	/* ##### RESULT RESTORE ##### */
	menuItem[total] = "Restore Previous Results";
	menuSubItem[total++] = NULL;

	/* ##### RETURN ##### */
	menuItem[total] = "Return to Prior Menu";
	menuSubItem[total++] = NULL;

	/* ##### TERMINATE ARRAY ##### */
	menuItem[total] = NULL;
	menuSubItem[total] = NULL;

	/* adjust current cursor position */
	ADJUST_CURSOR(menu->sel, total)

	/* draw it */
	old_style_menu(menuItem, menuSubItem, 0, menu->sel, 0);

	/********** KEY HANDLING **********/
	if(ui_pressed_repeat_throttle(machine, IPT_UI_UP, kVerticalKeyRepeatRate))
	{
		if(editActive)
		{
			switch(menu->sel)
			{
				case kMenu_Value:
				case kMenu_NearTo:
					search->old_options.value = DoIncrementHexField(search->old_options.value, editCursor);
					break;

				case kMenu_Increment:
					if(editCursor == 2)
						search->old_options.sign ^= 1;
					else
						search->old_options.delta = DoIncrementHexField(search->old_options.delta, editCursor);
					break;
			}
		}
		else
		{
			CURSOR_TO_PREVIOUS(menu->sel, total)

			if(menu->sel == kMenu_Separator) menu->sel--;
		}
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_DOWN, kVerticalKeyRepeatRate))
	{
		if(editActive)
		{
			switch(menu->sel)
			{
				case kMenu_Value:
				case kMenu_NearTo:
					search->old_options.value = DoDecrementHexField(search->old_options.value, editCursor);
					break;

				case kMenu_Increment:
					if(editCursor == 2)
						search->old_options.sign ^= 1;
					else
						search->old_options.delta = DoDecrementHexField(search->old_options.delta, editCursor);
					break;
			}
		}
		else
		{
			CURSOR_TO_NEXT(menu->sel, total)

			if(menu->sel == kMenu_Separator) menu->sel++;
		}
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_PAGE_UP, kVerticalKeyRepeatRate))
	{
		if(!editActive)
			CURSOR_PAGE_UP(menu->sel)
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_PAGE_DOWN, kVerticalKeyRepeatRate))
	{
		if(!editActive)
			CURSOR_PAGE_DOWN(menu->sel, total)
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_LEFT, kHorizontalFastKeyRepeatRate))
	{
		if(editActive)
		{
			switch(menu->sel)
			{
				case kMenu_Value:
				case kMenu_NearTo:
					if(++editCursor > 1) editCursor = 0;
					break;

				case kMenu_Increment:
					if(++editCursor > 2) editCursor = 0;
					break;
			}
		}
		else
		{
			switch(menu->sel)
			{
				case kMenu_Comparison:
				case kMenu_Bit:
					if(--search->old_options.comparison < kComparison_Equal)
						search->old_options.comparison = kComparison_Max;
					break;

				case kMenu_Value:
				case kMenu_NearTo:
					search->old_options.value = (search->old_options.value - 1) & kCheatSizeMaskTable[search->bytes];
					break;

				case kMenu_Increment:
					if(search->old_options.sign)
					{
						if(++search->old_options.delta > kSearchByteUnsignedMaskTable[search->bytes])
							search->old_options.delta = kSearchByteUnsignedMaskTable[search->bytes];
					}
					else if(--search->old_options.delta <= 0)
					{
						search->old_options.delta = 1;
						search->old_options.sign ^= 0x01;
					}
					break;
			}
		}
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_RIGHT, kHorizontalFastKeyRepeatRate))
	{
		if(editActive)
		{
			switch(menu->sel)
			{
				case kMenu_Value:
				case kMenu_NearTo:
					if(--editCursor < 0) editCursor = 1;
					break;

				case kMenu_Increment:
					if(--editCursor < 0) editCursor = 2;
					break;
			}
		}
		else
		{
			switch(menu->sel)
			{
				case kMenu_Comparison:
				case kMenu_Bit:
					if(++search->old_options.comparison > kComparison_GreaterOrEqual)
						search->old_options.comparison = 0;
					break;

				case kMenu_Value:
				case kMenu_NearTo:
					search->old_options.value = (search->old_options.value + 1) & kCheatSizeMaskTable[search->bytes];
					break;

				case kMenu_Increment:
					if(search->old_options.sign)
					{
						if(--search->old_options.delta <= 0)
						{
							search->old_options.delta = 1;
							search->old_options.sign ^= 0x01;
						}
					}
					else if(++search->old_options.delta > kSearchByteUnsignedMaskTable[search->bytes])
					{
						search->old_options.delta = kSearchByteUnsignedMaskTable[search->bytes];
					}
					break;
			}
		}
	}
	else if(input_ui_pressed(machine, IPT_UI_SELECT))
	{
		if(editActive)
			editActive = 0;
		else
		{
			switch(menu->sel)
			{
				case kMenu_Comparison:
					search->bytes		= kSearchSize_8Bit;
					search->rhs			= kSearchOperand_First;
					search->comparison	= kComparisonConversionTable[search->old_options.comparison];
					doSearch			= 1;
					break;

				case kMenu_Value:
					search->bytes		= kSearchSize_8Bit;
					search->rhs			= kSearchOperand_Value;
					search->comparison	= kComparisonConversionTable[search->old_options.comparison];
					search->value		= search->old_options.value;
					doSearch			= 1;
					break;

				case kMenu_NearTo:
					search->bytes		= kSearchSize_8Bit;
					search->rhs			= kSearchOperand_Value;
					search->comparison	= kSearchComparison_NearTo;
					search->value		= search->old_options.value;
					doSearch			= 1;
					break;

				case kMenu_Increment:
					if(search->old_options.delta)
					{
						search->bytes		= kSearchSize_8Bit;
						search->rhs			= kSearchOperand_First;
						search->comparison	= kSearchComparison_IncreasedBy;

						if(search->old_options.sign)
							search->value	= ~search->old_options.delta + 1;
						else
							search->value	= search->old_options.delta;

						doSearch			= 1;
					}
					break;

				case kMenu_Bit:
					search->bytes		= kSearchSize_1Bit;
					search->rhs			= kSearchOperand_First;
					search->comparison	= kComparisonConversionTable[search->old_options.comparison];
					doSearch			= 1;
					break;

				case kMenu_Memory:
					doneSaveMemory	= 0;
					doSearch		= 1;
					break;

				case kMenu_CPU:
					cheat_menu_stack_pop();
					break;

				case kMenu_ViewResult:
					if(search->region_list_length)
						cheat_menu_stack_push(view_search_result_menu, menu->handler, menu->sel);
					break;

				case kMenu_RestoreResult:
					restore_search_backup(search);
					break;

				case kMenu_Return:
					menu->sel = -1;
					break;
			}
		}
	}
	else if(input_ui_pressed(machine, IPT_UI_EDIT_CHEAT))
	{
		if(menu->sel == kMenu_Value || menu->sel == kMenu_NearTo || menu->sel == kMenu_Increment)
		{
			editActive ^= 1;
			editCursor = 0;
		}
	}
	else if(input_ui_pressed(machine, IPT_UI_CANCEL))
	{
		if(editActive)	editActive = 0;
		else			menu->sel = -1;
	}

	/***** SEARCH *****/
	if(doSearch)
	{
		if(!doneSaveMemory)
			init_new_search(search);

		if(search->rhs == kSearchOperand_Value || doneSaveMemory)
		{
			backup_search(search);
			do_search(search);
		}

		update_search(search);
		doneSaveMemory = 1;

		if(search->num_results == 1)
			AddCheatFromFirstResult(search);
	}

	/********** EDIT **********/
	switch(menu->sel)
	{
		case kMenu_Value:
			search->old_options.value = do_edit_hex_field(machine, search->old_options.value);
			search->old_options.value &= 0xFF;
			break;

		case kMenu_Increment:
			search->old_options.delta = do_edit_hex_field(machine, search->old_options.delta);
			search->old_options.delta &= 0x7F;
			break;
	}

	return menu->sel + 1;
}

/*-------------------------------------------------------------
  search_advanced_menu - management for advanced search menu
-------------------------------------------------------------*/

static int search_advanced_menu(running_machine *machine, cheat_menu_stack *menu)
{
#if 0
	/* menu stirngs */
	static const char *const kOperandNameTable[] =
	{
		"Current Data",
		"Previous Data",
		"First Data",
		"Value"
	};

	static const char *const kComparisonNameTable[] =
	{
		"Less",
		"Greater",
		"Equal",
		"Less Or Equal",
		"Greater Or Equal",
		"Not Equal",
		"Increased By Value",
		"Near To"
	};

	static const char *const kSearchByteNameTable[] =
	{
		"1",
		"2",
		"3",
		"4",
		"Bit"
	};

	enum
	{
		kMenu_LHS,
		kMenu_Comparison,
		kMenu_RHS,
		kMenu_Value,

		kMenu_Divider,

		kMenu_Size,
		kMenu_Swap,
		kMenu_Sign,
		kMenu_CPU,
		kMenu_Name,

		kMenu_Divider2,

		kMenu_DoSearch,
		kMenu_SaveMemory,

		kMenu_Divider3,

		kMenu_ViewResult,
		kMenu_RestoreSearch,

		kMenu_Return,

		kMenu_Max
	};

	const char		* menu_item[kMenu_Max + 2] =	{ 0 };
	const char		* menu_subitem[kMenu_Max + 2] =	{ 0 };

	char			valueBuffer[20];
	char			cpuBuffer[20];
	char			flagBuf[kMenu_Max + 2] = { 0 };

	search_info		*search = get_current_search();

	INT32			sel = selection - 1;
	INT32			total = 0;
	UINT32			increment = 1;

	static int		submenuChoice = 0;
	static UINT8	doneMemorySave = 0;
	static UINT8	editActive = 0;
	static UINT8	firstEntry = 0;
	static int		lastSel = 0;

	sel = lastSel;

	/********** SUB MENU **********/
	if(submenuChoice)
	{
		switch(sel)
		{
			case kMenu_CPU:
				submenuChoice = SelectSearchRegions(machine, submenuChoice, get_current_search());
				break;

			case kMenu_ViewResult:
				submenuChoice = ViewSearchResults(submenuChoice, firstEntry);
				break;
		}

		firstEntry = 0;

		/* meaningless ? because no longer return with sel = -1 (pressed UI_CONFIG in submenu) */
//      if(submenuChoice == -1)
//          submenuChoice = 0;

		return sel + 1;
	}

	/********** MENU CONSTRUCION **********/
	if((search->sign || search->comparison == kSearchComparison_IncreasedBy) && (search->value & kSearchByteSignBitTable[search->bytes]))
	{
		UINT32	tempValue;

		tempValue = ~search->value + 1;
		tempValue &= kSearchByteUnsignedMaskTable[search->bytes];

		sprintf(valueBuffer, "-%.*X", kSearchByteDigitsTable[search->bytes], tempValue);
	}
	else
		sprintf(valueBuffer, "%.*X", kSearchByteDigitsTable[search->bytes], search->value & kSearchByteMaskTable[search->bytes]);

	if(TEST_FIELD(cheatOptions, DontPrintNewLabels))
	{
		menu_item[total] = kOperandNameTable[search->lhs];
		menu_subitem[total++] = NULL;

		menu_item[total] = kComparisonNameTable[search->comparison];
		menu_subitem[total++] = NULL;

		menu_item[total] = kOperandNameTable[search->rhs];
		menu_subitem[total++] = NULL;

		menu_item[total] = valueBuffer;
		menu_subitem[total++] = NULL;
	}
	else
	{
		menu_item[total] = "LHS";
		menu_subitem[total++] = kOperandNameTable[search->lhs];

		menu_item[total] = "Comparison";
		menu_subitem[total++] = kComparisonNameTable[search->comparison];

		menu_item[total] = "RHS";
		menu_subitem[total++] = kOperandNameTable[search->rhs];

		menu_item[total] = "Value";
		menu_subitem[total++] = valueBuffer;
	}

	menu_item[total] = "---";
	menu_subitem[total++] = NULL;

	menu_item[total] = "Size";
	menu_subitem[total++] = kSearchByteNameTable[search->bytes];

	menu_item[total] = "Swap";
	menu_subitem[total++] = search->swap ? "On" : "Off";

	menu_item[total] = "Signed";
	menu_subitem[total++] = search->sign ? "On" : "Off";

	sprintf(cpuBuffer, "%d", search->targetIdx);
	menu_item[total] = "CPU";
	menu_subitem[total++] = cpuBuffer;

	menu_item[total] = "Name";
	if(search->name)
		menu_subitem[total++] = search->name;
	else
		menu_subitem[total++] = "(none)";

	menu_item[total] = "---";
	menu_subitem[total++] = NULL;

	menu_item[total] = "Do Search";
	menu_subitem[total++] = NULL;

	menu_item[total] = "Save Memory";
	menu_subitem[total++] = NULL;

	menu_item[total] = "---";
	menu_subitem[total++] = NULL;

	menu_item[total] = "View Last Results";		// view result
	menu_subitem[total++] = NULL;

	menu_item[total] = "Restore Previous Results";		// restore result
	menu_subitem[total++] = NULL;

	menu_item[total] = "Return to Prior Menu";		// return
	menu_subitem[total++] = NULL;

	menu_item[total] = NULL;								// terminate array
	menu_subitem[total] = NULL;

	/* adjust current cursor position */
	if(sel < 0)
		sel = 0;
	if(sel >= total)
		sel = total - 1;

	/* higlighted sub-item if edit mode */
	if(editActive)
		flagBuf[sel] = 1;

	/* draw it */
	old_style_menu(menu_item, menu_subitem, flagBuf, sel, 0);

	/********** KEY HANDLING **********/
	if(AltKeyPressed())
		increment <<= 4;
	if(ControlKeyPressed())
		increment <<= 8;
	if(ShiftKeyPressed())
		increment <<= 16;

	if(ui_pressed_repeat_throttle(machine, IPT_UI_DOWN, kVerticalKeyRepeatRate))
	{
		sel++;

		/* if divider, skip it */
		if((sel == kMenu_Divider) || (sel == kMenu_Divider2) || (sel == kMenu_Divider3))
			sel++;

		if(sel >= total)
			sel = 0;
	}

	if(ui_pressed_repeat_throttle(machine, IPT_UI_UP, kVerticalKeyRepeatRate))
	{
		sel--;

		/* if divider, skip it */
		if((sel == kMenu_Divider) || (sel == kMenu_Divider2) || (sel == kMenu_Divider3))
			sel--;

		if(sel < 0)
			sel = total - 1;
	}

	if(ui_pressed_repeat_throttle(machine, IPT_UI_PAGE_UP, kVerticalKeyRepeatRate))
	{
		sel -= fullMenuPageHeight;

		if(sel < 0)
			sel = 0;
	}

	if(ui_pressed_repeat_throttle(machine, IPT_UI_PAGE_DOWN, kVerticalKeyRepeatRate))
	{
		sel += fullMenuPageHeight;

		if(sel >= total)
			sel = total - 1;
	}

	if(ui_pressed_repeat_throttle(machine, IPT_UI_LEFT, kHorizontalFastKeyRepeatRate))
	{
		switch(sel)
		{
			case kMenu_Value:
				search->value -= increment;

				search->value &= kSearchByteMaskTable[search->bytes];
				break;

			case kMenu_LHS:
				search->lhs--;

				if(search->lhs < kSearchOperand_Current)
					search->lhs = kSearchOperand_Max;
				break;

			case kMenu_RHS:
				search->rhs--;

				if(search->rhs < kSearchOperand_Current)
					search->rhs = kSearchOperand_Max;
				break;

			case kMenu_Comparison:
				search->comparison--;

				if(search->comparison < kSearchComparison_LessThan)
					search->comparison = kSearchComparison_Max;
				break;

			case kMenu_Size:
				search->bytes--;

				if(search->bytes < kSearchSize_8Bit)
					search->bytes = kSearchSize_Max;
				break;

			case kMenu_Swap:
				search->swap ^= 1;
				break;

			case kMenu_Sign:
				search->sign ^= 1;
				break;

			case kMenu_CPU:
				if(search->targetIdx > 0)
				{
					search->targetIdx--;

					BuildSearchRegions(machine, search);
					AllocateSearchRegions(search);

					doneMemorySave = 0;
				}
				break;
		}
	}

	if(ui_pressed_repeat_throttle(machine, IPT_UI_RIGHT, kHorizontalFastKeyRepeatRate))
	{
		switch(sel)
		{
			case kMenu_Value:
				search->value += increment;

				search->value &= kSearchByteMaskTable[search->bytes];
				break;

			case kMenu_Size:
				search->bytes++;

				if(search->bytes > kSearchSize_Max)
					search->bytes = kSearchSize_8Bit;
				break;

			case kMenu_LHS:
				search->lhs++;

				if(search->lhs > kSearchOperand_Max)
					search->lhs = kSearchOperand_Current;
				break;

			case kMenu_RHS:
				search->rhs++;

				if(search->rhs > kSearchOperand_Max)
					search->rhs = kSearchOperand_Current;
				break;

			case kMenu_Comparison:
				search->comparison++;

				if(search->comparison > kSearchComparison_Max)
					search->comparison = kSearchComparison_LessThan;
				break;

			case kMenu_Swap:
				search->swap ^= 1;
				break;

			case kMenu_Sign:
				search->sign ^= 1;
				break;

			case kMenu_CPU:
				if(search->targetIdx < cpu_gettotalcpu() - 1)
				{
					search->targetIdx++;

					BuildSearchRegions(machine, search);
					AllocateSearchRegions(search);

					doneMemorySave = 0;
				}
				break;
		}
	}

	if(input_ui_pressed(machine, IPT_UI_SELECT))
	{
		if(editActive)
			editActive = 0;
		else
		{
			switch(sel)
			{
				case kMenu_Value:		// edit selected field
				case kMenu_Name:
					editActive = 1;
					break;

				case kMenu_CPU:			// go to region selection menu
					submenuChoice = 1;
					break;

				case kMenu_DoSearch:
					if(!doneMemorySave)
						/* initialize search */
						init_new_search(search);

					if((!kSearchOperandNeedsInit[search->lhs] && !kSearchOperandNeedsInit[search->rhs]) || doneMemorySave)
					{
						backup_search(search);

						DoSearch(search);
					}

					update_search(search);

					if((!kSearchOperandNeedsInit[search->lhs] && !kSearchOperandNeedsInit[search->rhs]) || doneMemorySave)
						popmessage("%d results found", search->num_results);
					else
						popmessage("saved all memory regions");

					doneMemorySave = 1;

					if(search->num_results == 1)
					{
						AddCheatFromFirstResult(search);

						popmessage("1 result found, added to list");
					}
					break;

				case kMenu_SaveMemory:
					init_new_search(search);

					update_search(search);

					popmessage("saved all memory regions");

					doneMemorySave = 1;
					break;

				case kMenu_ViewResult:
					if(search->regionListLength)
					{
						/* go to result viewer */
						firstEntry = 1;
						submenuChoice = 1;
					}
					else
						/* if no search region (eg, sms.c in HazeMD), don't open result viewer to avoid the crash */
						ui_popup_time(1, "no search regions");
					break;

				case kMenu_RestoreSearch:
					/* restore previous results */
					restore_search_backup(search);
					break;

				case kMenu_Return:
					submenuChoice = 0;
					sel = -1;
					break;

				default:
					break;
			}
		}
	}

	/* edit selected field  */
	if(editActive)
	{
		switch(sel)
		{
			case kMenu_Value:
				search->value = do_edit_hex_field(machine, search->value);

				search->value &= kSearchByteMaskTable[search->bytes];
				break;

			case kMenu_Name:
				search->name = DoDynamicEditTextField(search->name);
				break;
		}
	}

	if(input_ui_pressed(machine, IPT_UI_CANCEL))
		sel = -1;

	/* keep current cursor position */
	if(!(sel == -1))
		lastSel = sel;
#endif
	return menu->sel;
}

/*---------------------------------------------------------------------------
  select_search_region_menu - management for search regions selection menu
---------------------------------------------------------------------------*/

static int select_search_region_menu(running_machine *machine, cheat_menu_stack *menu)
{
	static const char *const ksearch_speedList[] =
	{
		"Fast",
		"Medium",
		"Slow",
		"Very Slow",
		"All Memory",
		"User Defined"
	};

	int				total		= 0;
	UINT8			doRebuild	= 0;
	const char		** menuItem;
	const char		** menuSubItem;
	search_info		*search = get_current_search();
	search_region	*region;

	/* first setting : NONE */
	if(menu->first_time)
		menu->first_time = 0;

	/* required items = speed + total regions + return + terminator */
	request_strings(search->region_list_length + 3, 0, 0, 0);
	menuItem	= menu_strings.main_list;
	menuSubItem	= menu_strings.sub_list;

	/********** MENU CONSTRUCTION **********/
	/* ##### SEARCH SPEED ##### */
	menuItem[total] = "Search Speed";
	menuSubItem[total++] = ksearch_speedList[search->search_speed];

	/* ##### REGIONS ##### */
	if(search->region_list_length)
	{
		int i;

		for(i = 0; i < search->region_list_length; i++)
		{
			search_region *region = &search->region_list[i];

			menuItem[total] = region->name;
			menuSubItem[total++] = region->flags & kRegionFlag_Enabled ? "On" : "Off";
		}
	}
	else
	{
		/* in case of no SearchRegion */
		menuItem[total] = "No Search Region";
		menuSubItem[total++] = NULL;
	}

	/* ##### RETURN ##### */
	menuItem[total] = "Return to Prior Menu";
	menuSubItem[total++] = NULL;

	/* ##### TERMINATE ARRAY ##### */
	menuItem[total] = NULL;
	menuSubItem[total] = NULL;

	/* adjust current cursor position */
	ADJUST_CURSOR(menu->sel, total)

	/* draw it */
	old_style_menu(menuItem, menuSubItem, NULL, menu->sel, 0);

	if(search->region_list_length && menu->sel && menu->sel < total -1)
		region = &search->region_list[menu->sel - 1];
	else
		region = NULL;

	/********** KEY HANDLING **********/
	if(ui_pressed_repeat_throttle(machine, IPT_UI_UP, kVerticalKeyRepeatRate))
	{
		CURSOR_TO_PREVIOUS(menu->sel, total)
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_DOWN, kVerticalKeyRepeatRate))
	{
		CURSOR_TO_NEXT(menu->sel, total)
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_PAGE_UP, kVerticalKeyRepeatRate))
	{
		CURSOR_PAGE_UP(menu->sel)
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_PAGE_DOWN, kVerticalKeyRepeatRate))
	{
		CURSOR_PAGE_DOWN(menu->sel, total)
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_LEFT, kHorizontalSlowKeyRepeatRate))
	{
		if(region)
		{
			/* toggle ON/OFF */
			region->flags ^= kRegionFlag_Enabled;
		}
		else if(!menu->sel)
		{
			if(--search->search_speed < 0)
				/* "Fast" -> "User Defined" */
				search->search_speed = ksearch_speed_Max - 1;

			doRebuild = 1;
		}
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_RIGHT, kHorizontalSlowKeyRepeatRate))
	{
		if(region)
		{
			/* toggle ON/OFF */
			region->flags ^= kRegionFlag_Enabled;
		}
		else if(!menu->sel)
		{
			if(++search->search_speed >= ksearch_speed_Max)
				/* "User Defined" -> "Fast" */
				search->search_speed = ksearch_speed_Fast;

			doRebuild = 1;
		}
	}
	else if(input_ui_pressed(machine, IPT_UI_DELETE_CHEAT))
	{
		if(ShiftKeyPressed() && region && search)
		{
			/* SHIFT + CHEAT DELETE = invalidate selected region */
			invalidate_entire_region(search, region);
//          ui_popup_time(1, "region invalidated - %d results remain", search->num_results);
		}
	}
	else if(input_ui_pressed(machine, IPT_UI_RELOAD_CHEAT))
	{
		if(search->search_speed == ksearch_speed_UserDefined)
		/* reload user region */
			doRebuild = 1;
	}
	else if(input_ui_pressed(machine, IPT_UI_SELECT))
	{
		if(region)
			region->flags ^= kRegionFlag_Enabled;
		else if(!menu->sel)
			doRebuild = 1;
		else
			menu->sel = -1;
	}
	else if(input_ui_pressed(machine, IPT_UI_CANCEL))
		menu->sel = -1;

	/********** REBUILD SEARCH REGION **********/
	if(doRebuild)
	{
		if(search->search_speed != ksearch_speed_UserDefined)
			/* rebuild SearchRegion from memory map */
			build_search_regions(machine, search);
		else
			/* rebuild SearchRegion from user-defined map */
			LoadCheatDatabase(machine, kLoadFlag_UserRegion);

		/* reallocate SearchRegions */
		allocate_search_regions(search);
	}

	return menu->sel + 1;
}

/*--------------------------------------------------------------
  view_search_result_menu - management for search result menu
--------------------------------------------------------------*/

static int view_search_result_menu(running_machine *machine, cheat_menu_stack *menu)
{
	enum{
		kMenu_Header = 0,
		kMenu_FirstResult,

		kMaxResultsPerPage = 100 };

	int				i;
	int				total				= 0;
	int				numPages;
	int				resultsPerPage;
	int				numToSkip;
	UINT8			hadResults			= 0;
	UINT8			pageSwitch			= 0;
	UINT8			resultsFound		= 0;
	UINT8			selectedAddressGood	= 0;
	UINT32			selectedAddress		= 0;
	UINT32			selectedOffset		= 0;
	UINT32			traverse;
	const char		** menu_item;
	char			** buf;
	char			* header;
	search_info		*search = get_current_search();
	search_region	*region;

	/* first setting : initialize region index and page number */
	if(menu->first_time)
	{
		search->current_region_idx = 0;
		search->current_results_page = 0;

		/* set current REGION for first display */
		for(traverse = 0; traverse < search->region_list_length; traverse++)
		{
			region = &search->region_list[traverse];

			if(region->num_results)
			{
				search->current_region_idx = traverse;
				break;
			}
		}

		menu->first_time = 0;
	}

	/* required items = (header + defined max items + return + terminator) & (strings buf * (header + defined max items) [max chars = 32]) */
	request_strings(kMaxResultsPerPage + 3, kMaxResultsPerPage, 32, 0);
	menu_item	= menu_strings.main_list;
	header		= menu_strings.main_strings[0];
	buf			= &menu_strings.main_strings[1];

	/* adjust current REGION */
	if(search->current_region_idx >= search->region_list_length)
		search->current_region_idx = search->region_list_length - 1;
	if(search->current_region_idx < 0)
		search->current_region_idx = 0;

	region = &search->region_list[search->current_region_idx];

	/* set the number of items per PAGE */
	resultsPerPage = fullMenuPageHeight - 3;

	/* adjust total items per PAGE */
	if(resultsPerPage <= 0)
		resultsPerPage = 1;
	else if(resultsPerPage > kMaxResultsPerPage)
		resultsPerPage = kMaxResultsPerPage;

	/* get the number of total PAGEs */
	if(region->flags & kRegionFlag_Enabled)
		numPages = (region->num_results / kSearchByteStep[search->bytes] + resultsPerPage - 1) / resultsPerPage;
	else
		numPages = 0;

	if(numPages > fullMenuPageHeight)
		numPages = fullMenuPageHeight;

	/* adjust current PAGE */
	if(search->current_results_page >= numPages)
		search->current_results_page = numPages - 1;
	if(search->current_results_page < 0)
		search->current_results_page = 0;

	/* set the number of skipping results to undisplay */
	numToSkip = resultsPerPage * search->current_results_page;

	/********** MENU CONSTRUCTION **********/
	/* ##### HEADER ##### */
	sprintf(header, "%s %d/%d", region->name, search->current_results_page + 1, numPages);

	menu_item[total++] = header;

	traverse = 0;

	if((region->length < kSearchByteIncrementTable[search->bytes]) || !(region->flags & kRegionFlag_Enabled))
	{
		; // no results...
	}
	else
	{
		/* ##### RESULT ##### */
		for(i = 0; i < resultsPerPage && traverse < region->length && resultsFound < region->num_results;)
		{
			while(is_region_offset_valid(search, region, traverse) == 0 && traverse < region->length)
				traverse += kSearchByteStep[search->bytes];

			if(traverse < region->length)
			{
				if(numToSkip > 0)
					numToSkip--;
				else
				{
					if(total == menu->sel)
					{
						selectedAddress		= region->address + traverse;
						selectedOffset		= traverse;
						selectedAddressGood	= 1;
					}

					sprintf(	buf[total],
								"%.8X (%.*X %.*X %.*X)",
								region->address + traverse,
								kSearchByteDigitsTable[search->bytes],
								read_search_operand(kSearchOperand_First, search, region, region->address + traverse),
								kSearchByteDigitsTable[search->bytes],
								read_search_operand(kSearchOperand_Previous, search, region, region->address + traverse),
								kSearchByteDigitsTable[search->bytes],
								read_search_operand(kSearchOperand_Current, search, region, region->address + traverse));

					menu_item[total] = buf[total];
					total++;

					i++;
				}

				traverse += kSearchByteStep[search->bytes];
				resultsFound++;
				hadResults = 1;
			}
		}
	}

	/* set special message if empty REGION */
	if(!hadResults)
	{
		if(search->num_results)
			menu_item[total++] = "no results for this region";
		else
			menu_item[total++] = "no results found";
	}

	/* ##### RETURN ##### */
	menu_item[total++] = "Return to Prior Menu";

	/* ##### TERMINATE ARRAY ##### */
	menu_item[total] = NULL;

	/* adjust current cursor position */
	if(menu->sel <= kMenu_Header)
		menu->sel = kMenu_FirstResult;
	if(menu->sel > total - 1 || !hadResults)
		menu->sel = total - 1;

	/* draw it */
	old_style_menu(menu_item, NULL, NULL, menu->sel, 0);

	/********** KEY HANDLING **********/
	if(ui_pressed_repeat_throttle(machine, IPT_UI_UP, kVerticalKeyRepeatRate))
	{
		if(--menu->sel < 1)
			menu->sel = total - 1;
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_DOWN, kVerticalKeyRepeatRate))
	{
		if(++menu->sel >= total)
			menu->sel = kMenu_FirstResult;
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_RIGHT, kHorizontalFastKeyRepeatRate))
	{
		if(ShiftKeyPressed())
		{
			/* shift + right = go to last PAGE */
			search->current_results_page = numPages - 1;
		}
		else if(ControlKeyPressed())
		{
			/* ctrl + right = go to next REGION */
			search->current_region_idx++;

			if(search->current_region_idx >= search->region_list_length)
				search->current_region_idx = 0;
		}
		else
		{
			/* otherwise, go to next PAGE */
			pageSwitch = 1;
		}

		menu->sel = kMenu_FirstResult;
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_LEFT, kHorizontalFastKeyRepeatRate))
	{
		if(ShiftKeyPressed())
		{
			/* shift + left = go to first PAGE */
			search->current_results_page = 0;
		}
		else if(ControlKeyPressed())
		{
			/* ctrl + left = go to previous REGION */
			search->current_region_idx--;

			if(search->current_region_idx < 0)
				search->current_region_idx = search->region_list_length - 1;
		}
		else
		{
			/* otherwise, go to previous PAGE */
			pageSwitch = 2;
		}

		menu->sel = kMenu_FirstResult;
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_PAGE_UP, kHorizontalFastKeyRepeatRate))
	{
		menu->sel -=fullMenuPageHeight;

		if(menu->sel <= kMenu_Header)
			menu->sel = kMenu_FirstResult;
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_PAGE_DOWN, kHorizontalFastKeyRepeatRate))
	{
		menu->sel +=fullMenuPageHeight;

		if(menu->sel >= total)
			menu->sel = total - 1;
	}
	else if(input_code_pressed_once(KEYCODE_HOME))
	{
		/* go to first PAGE */
		search->current_results_page = 0;
	}
	else if(input_code_pressed_once(KEYCODE_END))
	{
		/* go to last PAGE */
		search->current_results_page = numPages - 1;
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_PREV_GROUP, kVerticalKeyRepeatRate))
	{
		/* go to previous REGION */
		search->current_region_idx--;

		if(search->current_region_idx < 0)
			search->current_region_idx = search->region_list_length - 1;

		menu->sel = kMenu_FirstResult;
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_NEXT_GROUP, kVerticalKeyRepeatRate))
	{
		/* go to next REGION */
		search->current_region_idx++;

		if(search->current_region_idx >= search->region_list_length)
			search->current_region_idx = 0;

		menu->sel = kMenu_FirstResult;
	}
	else if(input_ui_pressed(machine, IPT_UI_ADD_CHEAT))
	{
		if(selectedAddressGood)
			AddCheatFromResult(search, region, selectedAddress);
	}
	else if(input_ui_pressed(machine, IPT_UI_DELETE_CHEAT))
	{
		if(ShiftKeyPressed())
		{
			if(region && search)
				/* shift + delete = invalidate all results in current REGION */
				invalidate_entire_region(search, region);
		}
		else if(selectedAddressGood)
		{
			/* otherwise, delete selected result */
			invalidate_region_offset(search, region, selectedOffset);
			search->num_results--;
			region->num_results--;

			messageType = kCheatMessage_SucceededToDelete;
			messageTimer = DEFAULT_MESSAGE_TIME;
		}
	}
	else if(input_ui_pressed(machine, IPT_UI_WATCH_VALUE))
	{
		if(selectedAddressGood)
			AddWatchFromResult(search, region, selectedAddress);
	}
	else if(input_ui_pressed(machine, IPT_UI_SELECT) || input_ui_pressed(machine, IPT_UI_CANCEL))
		menu->sel = -1;

	switch(pageSwitch)
	{
		case 1:		// go to next PAGE
			search->current_results_page++;

			/* if current PAGE is the last, go to first PAGE in next REGION */
			if(search->current_results_page >= numPages)
			{
				search->current_results_page = 0;
				search->current_region_idx++;

				/* if current REGION is the last, go to first REGION */
				if(search->current_region_idx >= search->region_list_length)
					search->current_region_idx = 0;

				/* if next REGION is empty, search next "non-empty" REGION.
                     but incomplete because last REGION is displayed even if empty */
				for(traverse = search->current_region_idx; traverse < search->region_list_length; traverse++)
				{
					search->current_region_idx = traverse;

					if(search->region_list[traverse].num_results)
						break;
				}
			}

			menu->sel = kMenu_FirstResult;
			break;

		case 2:		// go to previous PAGE
			search->current_results_page--;

			/* if current PAGE is first, go to previous REGION */
			if(search->current_results_page < 0)
			{
				search->current_results_page = 0;
				search->current_region_idx--;

				/* if current REGION is first, go to last REGION */
				if(search->current_region_idx < 0)
					search->current_region_idx = search->region_list_length - 1;

				/* if previous REGION is empty, search previous "non-empty" REGION.
                     but incomplete because first REGION is displayed even if empty */
				for(i = search->current_region_idx; i >= 0; i--)
				{
					search->current_region_idx = i;

					if(search->region_list[i].num_results)
						break;
				}

				/* go to last PAGE in previous REGION */
				{
					/* get the number of total PAGEs for previous REGION */
					search_region	*new_region		= &search->region_list[search->current_region_idx];
					UINT32			nextNumPages	= (new_region->num_results / kSearchByteStep[search->bytes] + resultsPerPage - 1) / resultsPerPage;

					if(nextNumPages <= 0)
						nextNumPages = 1;

					search->current_results_page = nextNumPages - 1;
				}
			}

			menu->sel = kMenu_FirstResult;
			break;
	}

	return menu->sel + 1;
}

/*----------------------------------------------------------
  choose_watch_menu - management for watchpoint list menu
----------------------------------------------------------*/

static int choose_watch_menu(running_machine *machine, cheat_menu_stack *menu)
{
	int				i;
	UINT8			total			= 0;
	const char		** menuItem;
	char			** buf;
	char			* stringsBuf;	// "USER20 FFFFFFFF (99:32 Bit)"    27 chars
	WatchInfo		* watch;

	/* first setting : NONE */
	if(menu->first_time)
		menu->first_time = 0;

	/* required items = (total watchpoints + return + terminator) & (strings buf * total watchpoints [max chars = 32]) */
	request_strings(watchListLength + 2, watchListLength, 32, 0);
	menuItem	= menu_strings.main_list;
	buf			= menu_strings.main_strings;

	/********** MENU CONSTRUCTION **********/
	for(i = 0; i < watchListLength; i++)
	{
		WatchInfo	* traverse	= &watchList[i];
		cpu_region_info *info = get_cpu_or_region_info(traverse->cpu);

		/* FORMAT : "CPU/Region : Address (Length : Bytes)" */
		if(!editActive || menu->sel != i)
		{
			sprintf(buf[i], "%s:%.*X (%d:%s)",
				get_region_name(traverse->cpu),
				info->address_chars_needed,
				traverse->address,
				traverse->numElements,
				kByteSizeStringList[traverse->elementBytes]);
		}
		else
		{
			/* insert edit cursor in quick edit mode */
			switch(editCursor)
			{
				default:
				{
					INT8 j;

					stringsBuf = buf[i];

					stringsBuf += sprintf(stringsBuf, "%s ", get_region_name(traverse->cpu));

					for(j = info->address_chars_needed + 1; j > 1; j--)
					{
						INT8 digits = j - 2;

						if(j == editCursor)
							stringsBuf += sprintf(stringsBuf, "[%X]", (traverse->address & kIncrementMaskTable[digits]) / kIncrementValueTable[digits]);
						else
							stringsBuf += sprintf(stringsBuf, "%X", (traverse->address & kIncrementMaskTable[digits]) / kIncrementValueTable[digits]);
					}

					stringsBuf += sprintf(stringsBuf, " (%d:%s)", traverse->numElements, kByteSizeStringList[traverse->elementBytes]);
				}
				break;

				case 1:
					sprintf(buf[i], "%s:%.*X ([%d]:%s)",
						get_region_name(traverse->cpu),
						info->address_chars_needed,
						traverse->address,
						traverse->numElements,
						kByteSizeStringList[traverse->elementBytes]);
					break;

				case 0:
					sprintf(buf[i], "%s:%.*X (%d:[%s])",
						get_region_name(traverse->cpu),
						info->address_chars_needed,
						traverse->address,
						traverse->numElements,
						kByteSizeStringList[traverse->elementBytes]);
					break;
			}
		}

		menuItem[total] = buf[i];
		total++;
	}

	/* ##### RETURN ##### */
	menuItem[total++] = "Return to Prior Menu";

	/* ##### TERMINATE ARRAY ##### */
	menuItem[total] = NULL;

	/* adjust current cursor position */
	ADJUST_CURSOR(menu->sel, total)

	/* draw it */
	old_style_menu(menuItem, NULL, NULL, menu->sel, 0);

	if(menu->sel < watchListLength)
		watch = &watchList[menu->sel];
	else
		watch = NULL;

	/********** KEY HANDLING **********/
	if(ui_pressed_repeat_throttle(machine, IPT_UI_UP, kVerticalKeyRepeatRate))
	{
		if(!editActive)
		{
			CURSOR_TO_PREVIOUS(menu->sel, total)
		}
		else
		{
			switch(editCursor)
			{
				default:
					watch->address = DoIncrementHexField(watch->address, editCursor - 2);
					break;

				case 1:
					if(watch->numElements < 99)
						watch->numElements++;
					break;

				case 0:
					watch->elementBytes = (watch->elementBytes + 1) & 0x03;
					break;
			}
		}
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_DOWN, kVerticalKeyRepeatRate))
	{
		if(!editActive)
		{
			CURSOR_TO_NEXT(menu->sel, total)
		}
		else
		{
			switch(editCursor)
			{
				default:
					watch->address = DoDecrementHexField(watch->address, editCursor - 2);
					break;

				case 1:
					if(watch->numElements > 0)
						watch->numElements--;
					break;

				case 0:
					watch->elementBytes = (watch->elementBytes - 1) & 0x03;
					break;
			}
		}
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_PAGE_UP, kVerticalKeyRepeatRate))
	{
		if(!editActive)
			CURSOR_PAGE_UP(menu->sel)
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_PAGE_DOWN, kVerticalKeyRepeatRate))
	{
		if(!editActive)
			CURSOR_PAGE_DOWN(menu->sel, total)
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_LEFT, kVerticalKeyRepeatRate))
	{
		if(editActive)
		{
			cpu_region_info *info = get_cpu_or_region_info(watch->cpu);

			if(++editCursor > info->address_chars_needed + 1)
				editCursor = 0;
		}
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_RIGHT, kVerticalKeyRepeatRate))
	{
		if(editActive)
		{
			cpu_region_info *info = get_cpu_or_region_info(watch->cpu);

			if(--editCursor < 0)
				editCursor = info->address_chars_needed + 1;
		}
	}
	else if(input_ui_pressed(machine, IPT_UI_SELECT))
	{
		if(editActive)
			editActive = 0;
		else
		{
			if(watch)
				cheat_menu_stack_push(command_watch_menu, menu->handler, menu->sel);
			else
				menu->sel = -1;
		}
	}
	else if(input_ui_pressed(machine, IPT_UI_ADD_CHEAT))
	{
		if(!editActive && watch)
		{
			if(ShiftKeyPressed())
				AddCheatFromWatch(watch);
			else if(ControlKeyPressed())
			{
				CheatEntry * entry = GetNewCheat();

				DisposeCheat(entry);
				SetupCheatFromWatchAsWatch(entry, watch);

				/* when fails to add, delete this entry because it causes the crash */
				if(messageType == kCheatMessage_FailedToAdd)
				{
					DeleteCheatAt(cheatListLength - 1);
					messageType		= kCheatMessage_FailedToAdd;
					messageTimer	= DEFAULT_MESSAGE_TIME;
				}
			}
			else
				AddWatchBefore(menu->sel);
		}
	}
	else if(input_ui_pressed(machine, IPT_UI_DELETE_CHEAT))
	{
		if(!editActive && watch)
		{
			if(ShiftKeyPressed())
			{
				DeleteWatchAt(menu->sel);
			}
			else if(ControlKeyPressed())
			{
				for(i = 0; i < watchListLength; i++)
					watchList[i].numElements = 0;
			}
			else
			{
				if(watch)
					watch->numElements = 0;
			}
		}
		else if(editActive)
		{
			switch(editCursor)
			{
				default:
					watch->address = 0;
					break;

				case 1:
					watch->numElements = 0;
					break;

				case 0:
					watch->elementBytes = 0;
			}
		}
	}
	else if(input_ui_pressed(machine, IPT_UI_SAVE_CHEAT))
	{
		if(!editActive && watch)
		{
			CheatEntry entry;

			memset(&entry, 0, sizeof(CheatEntry));

			SetupCheatFromWatchAsWatch(&entry, watch);
			SaveCheatCode(machine, &entry);
			DisposeCheat(&entry);
		}
	}
	else if(input_ui_pressed(machine, IPT_UI_EDIT_CHEAT))
	{
		if(watch)
		{
			editCursor = 2;
			editActive ^= 1;
		}
	}
	else if(input_ui_pressed(machine, IPT_UI_CLEAR))
	{
		if(!editActive)
		{
			if(ShiftKeyPressed())
			{
				if(watch)
					ResetWatch(watch);
			}
			else
			{
				for(i = 0; i < watchListLength; i++)
					ResetWatch(&watchList[i]);
			}
		}
		else
		{
			watch->address = 0;
			watch->numElements = 0;
			watch->elementBytes = 0;
		}
	}
	else if(input_ui_pressed(machine, IPT_UI_CANCEL))
	{
		if(editActive)
			editActive = 0;
		else
			menu->sel = -1;
	}

	return menu->sel + 1;
}

/*--------------------------------------------------------------
  command_watch_menu - management for wathcpoint command menu
--------------------------------------------------------------*/

static int command_watch_menu(running_machine *machine, cheat_menu_stack *menu)
{
	enum{
		kMenu_EditWatch = 0,
		kMenu_DisableWatch,
		kMenu_DisableAllWatch,
		kMenu_ResetWatch,
		kMenu_ResetAllWatch,
		kMenu_AddAsCheatCode,
		kMenu_AddAsWatchCode,
		kMenu_SaveAsCheatCode,
		kMenu_SaveAsWatchCode,
		kMenu_AddWatch,
		kMenu_DeleteWatch,
		kMenu_Return,
		kMenu_Max };

	int				i;
	UINT8			total	= 0;
	ui_menu_item	menuItem[kMenu_Max + 1];
	WatchInfo		* entry	= &watchList[menu->pre_sel];

	/* first setting : NONE */
	if(menu->first_time)
		menu->first_time = 0;

	memset(menuItem, 0, sizeof(menuItem));

	/********** MENU CONSTRUCION **********/
	menuItem[total++].text = "Edit Watchpoint";
	menuItem[total++].text = "Disable Watchpoint";
	menuItem[total++].text = "Disable All Watchpoints";
	menuItem[total++].text = "Reset Watchpoint";
	menuItem[total++].text = "Reset All Watchpoints";
	menuItem[total++].text = "Add as Cheat Code";
	menuItem[total++].text = "Add as Watch Code";
	menuItem[total++].text = "Save as Cheat Code";
	menuItem[total++].text = "Save as Watch Code";
	menuItem[total++].text = "Add New Watchpoint";
	menuItem[total++].text = "Delete Watchpoint";
	menuItem[total++].text = "Return to Prior Menu";
	menuItem[total].text = NULL;

	/* adjust current cursor position */
	ADJUST_CURSOR(menu->sel, total)

	/* print it */
	ui_menu_draw(menuItem, total, menu->sel, NULL);

	/********** KEY HANDLING **********/
	if(ui_pressed_repeat_throttle(machine, IPT_UI_UP, kVerticalKeyRepeatRate))
	{
		CURSOR_TO_PREVIOUS(menu->sel, total)
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_DOWN, kVerticalKeyRepeatRate))
	{
		CURSOR_TO_NEXT(menu->sel, total)
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_PAGE_UP, kVerticalKeyRepeatRate))
	{
		CURSOR_PAGE_UP(menu->sel)
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_PAGE_DOWN, kVerticalKeyRepeatRate))
	{
		CURSOR_PAGE_DOWN(menu->sel, total)
	}
	else if(input_ui_pressed(machine, IPT_UI_SELECT))
	{
		switch(menu->sel)
		{
			case kMenu_EditWatch:
				cheat_menu_stack_push(edit_watch_menu, menu->return_handler, menu->pre_sel);
				break;

			case kMenu_DisableWatch:
				entry->numElements = 0;
				break;

			case kMenu_DisableAllWatch:
				for(i = 0; i < watchListLength; i++)
					watchList[i].numElements = 0;
				break;

			case kMenu_ResetWatch:
				ResetWatch(entry);
				break;

			case kMenu_ResetAllWatch:
				for(i = 0; i < watchListLength; i++)
					ResetWatch(&watchList[i]);
				break;

			case kMenu_AddAsCheatCode:
				AddCheatFromWatch(entry);
				break;

			case kMenu_AddAsWatchCode:
			{
				CheatEntry * newEntry = GetNewCheat();

				DisposeCheat(newEntry);
				SetupCheatFromWatchAsWatch(newEntry, entry);

				/* when fails to add, delete this entry because it causes the crash */
				if(messageType == kCheatMessage_FailedToAdd)
				{
					DeleteCheatAt(cheatListLength - 1);
					messageType		= kCheatMessage_FailedToAdd;
					messageTimer	= DEFAULT_MESSAGE_TIME;
				}
			}
			break;

			case kMenu_SaveAsCheatCode:
				// underconstruction...
				break;

			case kMenu_SaveAsWatchCode:
			{
				CheatEntry tempEntry;

				memset(&tempEntry, 0, sizeof(CheatEntry));

				SetupCheatFromWatchAsWatch(&tempEntry, entry);
				SaveCheatCode(machine, &tempEntry);
				DisposeCheat(&tempEntry);
			}
			break;

			case kMenu_Return:
				menu->sel = -1;
		}
	}
	else if(input_ui_pressed(machine, IPT_UI_RELOAD_CHEAT))
	{
		ReloadCheatDatabase(machine);
	}
	else if(input_ui_pressed(machine, IPT_UI_CANCEL) || input_ui_pressed(machine, IPT_UI_LEFT) || input_ui_pressed(machine, IPT_UI_RIGHT))
	{
		menu->sel = -1;
	}

	return menu->sel + 1;
}

/*--------------------------------------------------------
  edit_watch_menu - management for watchpoint edit menu
--------------------------------------------------------*/

static int edit_watch_menu(running_machine *machine, cheat_menu_stack *menu)
{
	enum{
		kMenu_Address = 0,
		kMenu_CPU,
		kMenu_NumElements,
		kMenu_ElementSize,
		kMenu_LabelType,
		kMenu_TextLabel,
		kMenu_DisplayType,
		kMenu_XPosition,
		kMenu_YPosition,
		kMenu_Skip,
		kMenu_ElementsPerLine,
		kMenu_AddValue,
		kMenu_AddressShift,
		kMenu_DataShift,
		kMenu_XOR,

		kMenu_Return,
		kMenu_Max };

	UINT8			total		= 0;
	UINT32			increment	= 1;
	const char		** menuItem;
	const char		** menuSubItem;
	char			** buf;
	char			* flagBuf;
	WatchInfo		* entry		= &watchList[menu->pre_sel];
	cpu_region_info *info = get_cpu_or_region_info(entry->cpu);

	/* first setting : NONE */
	if(menu->first_time)
		menu->first_time = 0;

	request_strings(kMenu_Max + 1, kMenu_Return, 0, 20);
	menuItem		= menu_strings.main_list;
	menuSubItem		= menu_strings.sub_list;
	buf				= menu_strings.sub_strings;
	flagBuf			= menu_strings.flag_list;

	memset(flagBuf, 0, kMenu_Max + 1);

	/********** MENU CONSTRUCTION **********/
	/* ##### ADDRESS ##### */
	sprintf(buf[total], "%.*X", info->address_chars_needed, entry->address >> entry->addressShift);
	menuItem[total] = "Address";
	menuSubItem[total] = buf[total];
	total++;

	/* ##### CPU/REGION ##### */
	sprintf(buf[total], "%s", get_region_name(entry->cpu));
	menuItem[total] = TEST_FIELD(entry->cpu, RegionFlag) ? "Region" : "CPU";
	menuSubItem[total] = buf[total];
	total++;

	/* ##### LENGTH ##### */
	sprintf(buf[total], "%d", entry->numElements);
	menuItem[total] = "Length";
	menuSubItem[total] = buf[total];
	total++;

	/* ##### ELEMENT SIZE ##### */
	menuItem[total] = "Element Size";
	menuSubItem[total++] = kByteSizeStringList[entry->elementBytes];

	/* ##### LABEL TYPE ##### */
	menuItem[total] = "Label Type";
	menuSubItem[total++] = kWatchLabelStringList[entry->labelType];

	/* ##### TEXT LABEL ##### */
	menuItem[total] = "Text Label";
	if(entry->label[0])
		menuSubItem[total++] = entry->label;
	else
		menuSubItem[total++] = "(none)";

	/* ##### DISPLAY TYPE ##### */
	menuItem[total] = "Display Type";
	menuSubItem[total++] = kWatchDisplayTypeStringList[entry->displayType];

	/* ##### X POSITION ##### */
	sprintf(buf[total], "%f", entry->x);
	menuItem[total] = "X";
	menuSubItem[total] = buf[total];
	total++;

	/* ##### Y POSITION ##### */
	sprintf(buf[total], "%f", entry->y);
	menuItem[total] = "Y";
	menuSubItem[total] = buf[total];
	total++;

	/* ##### SKIP BYTES ##### */
	sprintf(buf[total], "%d", entry->skip);
	menuItem[total] = "Skip Bytes";
	menuSubItem[total] = buf[total];
	total++;

	/* ##### ELEMENTS PER LINE ##### */
	sprintf(buf[total], "%d", entry->elementsPerLine);
	menuItem[total] = "Elements Per Line";
	menuSubItem[total] = buf[total];
	total++;

	/* ##### ADD VALUE ##### */
	menuItem[total] = "Add Value";
	if(entry->addValue < 0)
		sprintf(buf[total], "-%.2X", -entry->addValue);
	else
		sprintf(buf[total], "%.2X", entry->addValue);
	menuSubItem[total] = buf[total];
	total++;

	/* ##### ADDRESS SHIFT ##### */
	sprintf(buf[total], "%d", entry->addressShift);
	menuItem[total] = "Address Shift";
	menuSubItem[total] = buf[total];
	total++;

	/* ##### DATA SHIFT ##### */
	sprintf(buf[total], "%d", entry->dataShift);
	menuItem[total] = "Data Shift";
	menuSubItem[total] = buf[total];
	total++;

	/* ##### XOR ##### */
	sprintf(buf[total], "%.*X", kSearchByteDigitsTable[kWatchSizeConversionTable[entry->elementBytes]], entry->xor);
	menuItem[total] = "XOR";
	menuSubItem[total] = buf[total];
	total++;

	/* ##### RETURN ##### */
	menuItem[total] = "Return to Prior Menu";
	menuSubItem[total++] = NULL;

	/* ##### TERMINATE ARRAY ##### */
	menuItem[total] = NULL;
	menuSubItem[total] = NULL;

	/* adjust current cursor position */
	ADJUST_CURSOR(menu->sel, total)

	/* higlighted sub-item if edit mode */
	if(editActive)
		flagBuf[menu->sel] = 1;

	/* draw it */
	old_style_menu(menuItem, menuSubItem, flagBuf, menu->sel, 0);

	/********** KEY HANDLING **********/
	if(ui_pressed_repeat_throttle(machine, IPT_UI_UP, kVerticalKeyRepeatRate))
	{
		CURSOR_TO_PREVIOUS(menu->sel, total)
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_DOWN, kVerticalKeyRepeatRate))
	{
		CURSOR_TO_NEXT(menu->sel, total)
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_PAGE_UP, kVerticalKeyRepeatRate))
	{
		CURSOR_PAGE_UP(menu->sel)
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_PAGE_DOWN, kVerticalKeyRepeatRate))
	{
		CURSOR_PAGE_DOWN(menu->sel, total)
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_LEFT, kHorizontalSlowKeyRepeatRate))
	{
		if(editActive)
		{
			// underconstruction...
		}
		else
		{
			switch(menu->sel)
			{
				case kMenu_Address:
					entry->address = DoShift(entry->address, entry->addressShift);
					entry->address -= increment;
					entry->address = DoShift(entry->address, -entry->addressShift);

					if(!TEST_FIELD(entry->cpu, RegionFlag))
						entry->address &= info->address_mask;
					break;

				case kMenu_CPU:
					entry->cpu--;

					if(entry->cpu >= cpu_gettotalcpu())
						entry->cpu = cpu_gettotalcpu() - 1;

					entry->address &= info->address_mask;
					break;

				case kMenu_NumElements:
					if(entry->numElements > 0)
						entry->numElements--;
					else
						entry->numElements = 0;
					break;

				case kMenu_ElementSize:
					if(entry->elementBytes > 0)
						entry->elementBytes--;
					else
						entry->elementBytes = 0;

					entry->xor &= kSearchByteMaskTable[kWatchSizeConversionTable[entry->elementBytes]];
					break;

				case kMenu_LabelType:
					if(entry->labelType > 0)
						entry->labelType--;
					else
						entry->labelType = 0;
					break;

				case kMenu_TextLabel:
					break;

				case kMenu_DisplayType:
					if(entry->displayType > 0)
						entry->displayType--;
					else
						entry->displayType = 0;
					break;

				case kMenu_XPosition:
					entry->x -= 0.01f;
					break;

				case kMenu_YPosition:
					entry->y -= 0.01f;
					break;

				case kMenu_Skip:
					if(entry->skip > 0)
						entry->skip--;
					break;

				case kMenu_ElementsPerLine:
					if(entry->elementsPerLine > 0)
						entry->elementsPerLine--;
					break;

				case kMenu_AddValue:
					entry->addValue = (entry->addValue - 1) & 0xFF;
					break;

				case kMenu_AddressShift:
					if(entry->addressShift > -31)
						entry->addressShift--;
					else
						entry->addressShift = 31;
					break;

				case kMenu_DataShift:
					if(entry->dataShift > -31)
						entry->dataShift--;
					else
						entry->dataShift = 31;
					break;

				case kMenu_XOR:
					entry->xor -= increment;
					entry->xor &= kSearchByteMaskTable[kWatchSizeConversionTable[entry->elementBytes]];
			}
		}
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_RIGHT, kHorizontalSlowKeyRepeatRate))
	{
		if(editActive)
		{
			// underconstruction...
		}
		else
		{
			switch(menu->sel)
			{
				case kMenu_Address:
					entry->address = DoShift(entry->address, entry->addressShift);
					entry->address += increment;
					entry->address = DoShift(entry->address, -entry->addressShift);

					if(!TEST_FIELD(entry->cpu, RegionFlag))
						entry->address &= info->address_mask;
					break;

				case kMenu_CPU:
					entry->cpu++;

					if(entry->cpu >= cpu_gettotalcpu())
						entry->cpu = 0;

					entry->address &= info->address_mask;
					break;

				case kMenu_NumElements:
					entry->numElements++;
					break;

				case kMenu_ElementSize:
					if(entry->elementBytes < kSearchSize_32Bit)
						entry->elementBytes++;
					else
						entry->elementBytes = kSearchSize_32Bit;

					entry->xor &= kSearchByteMaskTable[kWatchSizeConversionTable[entry->elementBytes]];
					break;

				case kMenu_LabelType:
					if(entry->labelType < kWatchLabel_MaxPlusOne - 1)
						entry->labelType++;
					else
						entry->labelType = kWatchLabel_MaxPlusOne - 1;
					break;

				case kMenu_TextLabel:
					break;

				case kMenu_DisplayType:
					if(entry->displayType < kWatchDisplayType_MaxPlusOne - 1)
						entry->displayType++;
					else
						entry->displayType = kWatchDisplayType_MaxPlusOne - 1;
					break;

				case kMenu_XPosition:
					entry->x += 0.01f;
					break;

				case kMenu_YPosition:
					entry->y += 0.01f;
					break;

				case kMenu_Skip:
					entry->skip++;
					break;

				case kMenu_ElementsPerLine:
					entry->elementsPerLine++;
					break;

				case kMenu_AddValue:
					entry->addValue = (entry->addValue + 1) & 0xFF;
					break;

				case kMenu_AddressShift:
					if(entry->addressShift < 31)
						entry->addressShift++;
					else
						entry->addressShift = -31;
					break;

				case kMenu_DataShift:
					if(entry->dataShift < 31)
						entry->dataShift++;
					else
						entry->dataShift = -31;
					break;

				case kMenu_XOR:
					entry->xor += increment;
					entry->xor &= kSearchByteMaskTable[kWatchSizeConversionTable[entry->elementBytes]];
			}
		}
	}
	else if(input_ui_pressed(machine, IPT_UI_SELECT))
	{
		if(editActive)
			editActive = 0;
		else
		{
			switch(menu->sel)
			{
				case kMenu_Address:
				case kMenu_CPU:
				case kMenu_NumElements:
				case kMenu_TextLabel:
				case kMenu_XPosition:
				case kMenu_YPosition:
				case kMenu_AddValue:
				case kMenu_AddressShift:
				case kMenu_DataShift:
				case kMenu_XOR:
					osd_readkey_unicode(1);
					editActive = 1;
					break;

				case kMenu_Return:
					menu->sel = -1;
			}
		}
	}
	else if(input_ui_pressed(machine, IPT_UI_CLEAR))
	{
		ResetWatch(entry);
	}
	else if(input_ui_pressed(machine, IPT_UI_CANCEL))
	{
		if(editActive)
			editActive = 0;
		else
			menu->sel = -1;
	}

	if(editActive)
	{
		switch(menu->sel)
		{
			case kMenu_Address:
				entry->address = DoShift(entry->address, entry->addressShift);
				entry->address = do_edit_hex_field(machine, entry->address);
				entry->address = DoShift(entry->address, -entry->addressShift);
				entry->address &= info->address_mask;
				break;

			case kMenu_CPU:
				entry->cpu = DoEditDecField(entry->cpu, 0, cpu_gettotalcpu() - 1);
				entry->address &= info->address_mask;
				break;

			case kMenu_NumElements:
				entry->numElements = DoEditDecField(entry->numElements, 0, 99);
				break;

			case kMenu_TextLabel:
				DoStaticEditTextField(entry->label, 255);
				break;

			case kMenu_XPosition:
				entry->x = DoEditDecField(entry->x, -1000, 1000);
				break;

			case kMenu_YPosition:
				entry->y = DoEditDecField(entry->y, -1000, 1000);
				break;

			case kMenu_AddValue:
				entry->addValue = DoEditHexFieldSigned(entry->addValue, 0xFFFFFF80) & 0xFF;
				break;

			case kMenu_AddressShift:
				entry->addressShift = DoEditDecField(entry->addressShift, -31, 31);
				break;

			case kMenu_DataShift:
				entry->dataShift = DoEditDecField(entry->dataShift, -31, 31);
				break;

			case kMenu_XOR:
				entry->xor = do_edit_hex_field(machine, entry->xor);
				entry->xor &= kSearchByteMaskTable[kWatchSizeConversionTable[entry->elementBytes]];
				break;
		}
	}
	else
	{
		if(input_ui_pressed(machine, IPT_UI_ADD_CHEAT))
			AddCheatFromWatch(entry);

		if(input_ui_pressed(machine, IPT_UI_DELETE_CHEAT))
			entry->numElements = 0;

		if(input_ui_pressed(machine, IPT_UI_SAVE_CHEAT))
		{
			CheatEntry tempEntry;

			memset(&tempEntry, 0, sizeof(CheatEntry));

			SetupCheatFromWatchAsWatch(&tempEntry, entry);
			SaveCheatCode(machine, &tempEntry);
			DisposeCheat(&tempEntry);
		}
	}

	return menu->sel + 1;
}

/*---------------------------------------------------
  select_option_menu - management for options menu
---------------------------------------------------*/

static int select_option_menu(running_machine *machine, cheat_menu_stack *menu)
{
	enum{
		kMenu_SelectSearch = 0,
		kMenu_LoadCheatOptions,
		kMenu_SaveCheatOptions,
		kMenu_ResetCheatOptions,
		kMenu_Separator,
		kMenu_SearchDialogStyle,
		kMenu_ShowSearchLabels,
		kMenu_AutoSaveCheats,
		kMenu_ShowActivationKeyMessage,
		kMenu_LoadOldFormat,
#ifdef MESS
		kMenu_SharedCode,
#endif
		kMenu_Return,

		kMenu_Max };

	UINT8			total		= 0;
	const char		* menu_item[kMenu_Max + 1];
	const char		* menu_sub_item[kMenu_Max + 1];

	/* first setting : NONE */
	if(menu->first_time)
		menu->first_time = 0;

	/********** MENU CONSTRUCTION **********/
	/* ##### SEARCH REGION ##### */
	ADD_MENU_2_ITEMS("Select Search", NULL);

	/* ##### LOAD CHEAT OPTIONS ##### */
	ADD_MENU_2_ITEMS("Load Cheat Options", NULL);

	/* ##### SAVE CHEAT OPTIONS ##### */
	ADD_MENU_2_ITEMS("Save Cheat Options", NULL);

	/* ##### RESET CHEAT OPTIONS ##### */
	ADD_MENU_2_ITEMS("Reset Cheat Options", NULL);

	/* ##### SEPARATOR ##### */
	ADD_MENU_2_ITEMS(MENU_SEPARATOR_ITEM, NULL);

	/* ##### SEARCH MENU ##### */
	menu_item[total] = "Search Dialog Style";
	switch(EXTRACT_FIELD(cheatOptions, SearchBox))
	{
		case kSearchBox_Minimum:
			menu_sub_item[total++] = "Minimum";
			break;

		case kSearchBox_Standard:
			menu_sub_item[total++] = "Standard";
			break;

		case kSearchBox_Advanced:
			menu_sub_item[total++] = "Advanced";
			break;

		default:
			menu_sub_item[total++] = "<< No Search Box >>";
			break;
	}

	/* ##### SEARCH LABEL ##### */
	ADD_MENU_2_ITEMS("Show Search Labels", TEST_FIELD(cheatOptions, DontPrintNewLabels) ? "Off" : "On");

	/* ##### AUTO SAVE ##### */
	ADD_MENU_2_ITEMS("Auto Save Cheats", TEST_FIELD(cheatOptions, AutoSaveEnabled) ? "On" : "Off");

	/* ##### ACTIVATION KEY MESSAGE ##### */
	ADD_MENU_2_ITEMS("Show Activation Key Message", TEST_FIELD(cheatOptions, ActivationKeyMessage) ? "On" : "Off");

	/* ##### OLD FORMAT LOADING ##### */
	ADD_MENU_2_ITEMS("Load Old Format Code", TEST_FIELD(cheatOptions, LoadOldFormat) ? "On" : "Off");

#ifdef MESS
	/* ##### SHARED CODE ##### */
	ADD_MENU_2_ITEMS("Shared Code", TEST_FIELD(cheatOptions, SharedCode) ? "On" : "Off");
#endif

	/* ##### RETURN ##### */
	ADD_MENU_2_ITEMS("Return to Prior Menu", NULL);

	/* ##### TERMINATE ALLAY ##### */
	TERMINATE_MENU_2_ITEMS;

	/* draw it */
	old_style_menu(menu_item, menu_sub_item, NULL, menu->sel, 0);

	/********** KEY HANDLING **********/
	if(ui_pressed_repeat_throttle(machine, IPT_UI_UP, kVerticalKeyRepeatRate))
	{
		CURSOR_TO_PREVIOUS(menu->sel, total)

		if(menu->sel == kMenu_Separator)
			menu->sel--;
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_DOWN, kVerticalKeyRepeatRate))
	{
		CURSOR_TO_NEXT(menu->sel, total)

		if(menu->sel == kMenu_Separator)
			menu->sel++;
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_PAGE_UP, kVerticalKeyRepeatRate))
	{
		CURSOR_PAGE_UP(menu->sel)
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_PAGE_DOWN, kVerticalKeyRepeatRate))
	{
		CURSOR_PAGE_DOWN(menu->sel, total)
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_LEFT, kHorizontalSlowKeyRepeatRate))
	{
		switch(menu->sel)
		{
			case kMenu_SearchDialogStyle:
			{
				UINT8 length = (EXTRACT_FIELD(cheatOptions, SearchBox) - 1) & 3;

				if(length > kSearchBox_Advanced)
					length = kSearchBox_Advanced;

				SET_FIELD(cheatOptions, SearchBox, length);
			}
			break;

			case kMenu_ShowSearchLabels:
				TOGGLE_MASK_FIELD(cheatOptions, DontPrintNewLabels);
				break;

			case kMenu_AutoSaveCheats:
				TOGGLE_MASK_FIELD(cheatOptions, AutoSaveEnabled);
				break;

			case kMenu_ShowActivationKeyMessage:
				TOGGLE_MASK_FIELD(cheatOptions, ActivationKeyMessage);
				break;

			case kMenu_LoadOldFormat:
				TOGGLE_MASK_FIELD(cheatOptions, LoadOldFormat);
				break;
#ifdef MESS
			case kMenu_SharedCode:
				TOGGLE_MASK_FIELD(cheatOptions, SharedCode);
#endif
		}
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_RIGHT, kHorizontalSlowKeyRepeatRate))
	{
		switch(menu->sel)
		{
			case kMenu_SearchDialogStyle:
			{
				UINT8 length = (EXTRACT_FIELD(cheatOptions, SearchBox) + 1) & 3;

				if(length > kSearchBox_Advanced)
					length = kSearchBox_Minimum;

				SET_FIELD(cheatOptions, SearchBox, length);
			}
			break;

			case kMenu_ShowSearchLabels:
				TOGGLE_MASK_FIELD(cheatOptions, DontPrintNewLabels);
				break;

			case kMenu_AutoSaveCheats:
				TOGGLE_MASK_FIELD(cheatOptions, AutoSaveEnabled);
				break;

			case kMenu_ShowActivationKeyMessage:
				TOGGLE_MASK_FIELD(cheatOptions, ActivationKeyMessage);
				break;

			case kMenu_LoadOldFormat:
				TOGGLE_MASK_FIELD(cheatOptions, LoadOldFormat);

#ifdef MESS
			case kMenu_SharedCode:
				TOGGLE_MASK_FIELD(cheatOptions, SharedCode);
#endif
		}
	}
	else if(input_ui_pressed(machine, IPT_UI_SELECT))
	{
		switch(menu->sel)
		{
			case kMenu_SelectSearch:
				cheat_menu_stack_push(select_search_menu, menu->handler, menu->sel);
				break;

			case kMenu_LoadCheatOptions:
				LoadCheatDatabase(machine, kLoadFlag_CheatOption);
				break;

			case kMenu_SaveCheatOptions:
				SaveCheatOptions();
				break;

			case kMenu_ResetCheatOptions:
				cheatOptions = DEFAULT_CHEAT_OPTIONS;
				break;

			case kMenu_Return:
				menu->sel = -1;
		}
	}
	else if(input_ui_pressed(machine, IPT_UI_SAVE_CHEAT))
	{
		SaveCheatOptions();
	}
	else if(input_ui_pressed(machine, IPT_UI_RELOAD_CHEAT))
	{
		LoadCheatDatabase(machine, kLoadFlag_CheatOption);
	}
	else if(input_ui_pressed(machine, IPT_UI_CANCEL))
	{
		menu->sel = -1;
	}

	return menu->sel + 1;
}

/*------------------------------------------------------------
  select_search_menu - management for search selection menu
------------------------------------------------------------*/

static int select_search_menu(running_machine *machine, cheat_menu_stack *menu)
{
#if 0
	INT32			sel;
	const char		** menuItem;
	char			** buf;
	INT32			i;
	INT32			total = 0;

	sel = selection - 1;

	request_strings(search_list_length + 2, search_list_length, 300, 0);

	menuItem =	menu_strings.main_list;
	buf =		menu_strings.main_strings;

	/********** MENU CONSTRUCTION **********/
	for(i = 0; i < search_list_length; i++)
	{
		search_info *info = &search_list[i];

		if(i == current_search_idx)
		{
			if(info->name)
				sprintf(buf[total], "[#%d: %s]", i, info->name);
			else
				sprintf(buf[total], "[#%d]", i);
		}
		else
		{
			if(info->name)
				sprintf(buf[total], "#%d: %s", i, info->name);
			else
				sprintf(buf[total], "#%d", i);
		}

		menuItem[total] = buf[total];
		total++;
	}

	menuItem[total++] = "Return to Prior Menu";		// return

	menuItem[total] = NULL;									// terminate array

	/* adjust current cursor position */
	if(sel < 0)
		sel = 0;
	if(sel > (total - 1))
		sel = total - 1;

	/* draw it */
	old_style_menu(menuItem, NULL, NULL, sel, 0);

	/********** KEY HANDLING **********/
	if(ui_pressed_repeat_throttle(machine, IPT_UI_DOWN, kVerticalKeyRepeatRate))
	{
		sel++;

		if(sel >= total)
			sel = 0;
	}

	if(ui_pressed_repeat_throttle(machine, IPT_UI_UP, kVerticalKeyRepeatRate))
	{
		sel--;

		if(sel < 0)
			sel = total - 1;
	}

	if(ui_pressed_repeat_throttle(machine, IPT_UI_PAGE_UP, kVerticalKeyRepeatRate))
	{
		sel -= fullMenuPageHeight;

		if(sel < 0)
			sel = 0;
	}

	if(ui_pressed_repeat_throttle(machine, IPT_UI_PAGE_DOWN, kVerticalKeyRepeatRate))
	{
		sel += fullMenuPageHeight;

		if(sel >= total)
			sel = total - 1;
	}

	if(input_ui_pressed(machine, IPT_UI_ADD_CHEAT))
	{
		AddSearchBefore(sel);

		BuildSearchRegions(machine, &searchList[sel]);
		AllocateSearchRegions(&searchList[sel]);
	}

	if(input_ui_pressed(machine, IPT_UI_DELETE_CHEAT))
	{
		if(search_list_length > 1)
			DeleteSearchAt(sel);
	}

	if(input_ui_pressed(machine, IPT_UI_EDIT_CHEAT))
	{
		if(sel < total - 1)
			current_search_idx = sel;
	}

	if(input_ui_pressed(machine, IPT_UI_SELECT))
	{
		if(sel >= total - 1)
			sel = -1;
		else
			current_search_idx = sel;
	}

	if(input_ui_pressed(machine, IPT_UI_CANCEL))
		sel = -1;
#endif
	return menu->sel;
}

#ifdef MAME_DEBUG
/*-----------------------------------------------------------------------
  check_activation_key_code_menu - key code checker for activation key
-----------------------------------------------------------------------*/

static int check_activation_key_code_menu(running_machine *machine, cheat_menu_stack *menu)
{
	int				code		= input_code_poll_switches(FALSE);
	static int		index		= INPUT_CODE_INVALID;
	char			stringsBuf[64];
	char			* buf		= stringsBuf;
	astring			* keyIndex	= astring_alloc();

	if(code != INPUT_CODE_INVALID)
	{
		/* NOTE : if first, no action to prevent from wrong display */
		if(menu->first_time)
			menu->first_time = 0;
		else
			index = code;
	}

	/********** MENU CONSTRUCTION **********/
	/* ##### HEADER ##### */
	buf += sprintf(buf, "\tPress a key\n");

	/* ##### KEY NAME / INDEX ##### */
	if(index != INPUT_CODE_INVALID && input_code_pressed(index))
		buf += sprintf(buf, "\t%s\n\t%X\n", astring_c(input_code_name(keyIndex, index)), index);
	else
		buf += sprintf(buf, "\n\n");

	/* ##### RETURN ##### */
	buf += sprintf(buf, "\t Return = Shift + Cancel ");

	/* print it */
	ui_draw_message_window(stringsBuf);

	/* NOTE : shift + cancel is only key to return because normal cancel prevents from diplaying this key */
	if(ShiftKeyPressed() && input_ui_pressed(machine, IPT_UI_CANCEL))
	{
		index = INPUT_CODE_INVALID;
		menu->sel = -1;
	}

	astring_free(keyIndex);

	return menu->sel + 1;
}

/*------------------------------------------------------------------------------------------------
  view_cpu_region_info_list_menu - view internal CPU or Region info list called from debug menu
------------------------------------------------------------------------------------------------*/

static int view_cpu_region_info_list_menu(running_machine *machine, cheat_menu_stack *menu)
{
	enum{
		kMenu_Header = 0,
		kMenu_Type,
		kMenu_DataBits,
		kMenu_AddressBites,
		kMenu_AddressMask,
		kMenu_Endianness,
		kMenu_AddressShift,
		kMenu_Return,
		kMenu_Max };

	UINT8		total	= 0;
	static INT8	index;
	const char	** menuItem;
	const char	** menuSubItem;
	char		* headerBuf;
	char		* regionNameBuf;
	char		* dataBitsBuf;
	char		* addressBitsBuf;
	char		* addressMaskBuf;
	char		* addressShiftBuf;
	cpu_region_info *info;

	/* required items = total items + (strings buf * 6 [max chars : 32]) */
	request_strings(kMenu_Max, kMenu_Max - 2, 32, 0);
	menuItem 		= menu_strings.main_list;
	menuSubItem		= menu_strings.sub_list;
	headerBuf		= menu_strings.main_strings[0];
	regionNameBuf	= menu_strings.main_strings[1];
	dataBitsBuf		= menu_strings.main_strings[2];
	addressBitsBuf	= menu_strings.main_strings[3];
	addressMaskBuf	= menu_strings.main_strings[4];
	addressShiftBuf	= menu_strings.main_strings[5];

	/* first setting : set index as 1st */
	if(menu->first_time)
	{
		index = REGION_CPU1 - REGION_INVALID;
		menu->first_time = 0;
	}

	info = get_region_info(index + REGION_INVALID);

	/********** MENU CONSTRUCTION **********/
	/* ##### HEADER ##### */
	menuItem[total] = "Index";
	sprintf(headerBuf, "%2.2d", index);
	menuSubItem[total++] = headerBuf;

	/* ##### CPU/REGION TYPE ##### */
	menuItem[total] = "Region";

	if(index < REGION_CPU8 - REGION_INVALID)
		sprintf(regionNameBuf, "%s (%s)", kRegionNames[index], cputype_name(info->type));
	else
		sprintf(regionNameBuf, "%s (non-cpu)", kRegionNames[info->type - REGION_CPU1 + 1]);
	menuSubItem[total++] = regionNameBuf;

	/* ##### DATA BITS ##### */
	menuItem[total] = "Data Bits";
	sprintf(dataBitsBuf, "%d", info->data_bits);
	menuSubItem[total++] = dataBitsBuf;

	/* ##### ADDRESS BITS ##### */
	menuItem[total] = "Address Bits";
	sprintf(addressBitsBuf, "%d", info->address_bits);
	menuSubItem[total++] = addressBitsBuf;

	/* ##### ADDRESS MASK/LENGTH ##### */
	menuItem[total] = "Address Mask";
	sprintf(addressMaskBuf, "%X (%d)", info->address_mask, info->address_chars_needed);
	menuSubItem[total++] = addressMaskBuf;

	/* ##### ENDIANNESS ##### */
	menuItem[total] = "Endianness";
	menuSubItem[total++] = info->endianness ? "Big" : "Little";

	/* ##### ADDRESS SHIFT ##### */
	menuItem[total] = "AddressShift";
	sprintf(addressShiftBuf, "%d", info->address_shift);
	menuSubItem[total++] = addressShiftBuf;

	/* ##### RETURN ##### */
	menuItem[total] = "Return to Prior Menu";
	menuSubItem[total++] = NULL;

	/* ##### TERMINATE ARRAY ##### */
	menuItem[total] = NULL;
	menuSubItem[total] = NULL;

	/* draw it */
	old_style_menu(menuItem, menuSubItem, NULL, menu->sel, 0);

	/* adjust current cursor position */
	ADJUST_CURSOR(menu->sel, total)

	/********** KEY HANDLING **********/
	if(ui_pressed_repeat_throttle(machine, IPT_UI_UP, kVerticalKeyRepeatRate))
	{
		CURSOR_TO_PREVIOUS(menu->sel, total)
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_DOWN, kVerticalKeyRepeatRate))
	{
		CURSOR_TO_NEXT(menu->sel, total)
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_PAGE_UP, kVerticalKeyRepeatRate))
	{
		CURSOR_PAGE_UP(menu->sel)
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_PAGE_DOWN, kVerticalKeyRepeatRate))
	{
		CURSOR_PAGE_DOWN(menu->sel, total)
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_RIGHT, kHorizontalSlowKeyRepeatRate))
	{
		int i = index + 1;

		/* search next valid info list */
		while(1)
		{
			if(i >= REGION_LIST_LENGTH - 1)
			{
				i = 0;
				continue;
			}

			if(region_info_list[i].type)
			{
				index = i;
				break;
			}
			else
				i++;
		}
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_LEFT, kHorizontalSlowKeyRepeatRate))
	{
		int i = index - 1;

		/* search previous valid info list */
		while(1)
		{
			if(i < 0)
			{
				i = REGION_LIST_LENGTH - 1;
				continue;
			}

			if(region_info_list[i].type)
			{
				index = i;
				break;
			}
			else
				i--;
		}
	}
	else if(input_ui_pressed(machine, IPT_UI_SELECT))
	{
		if(menu->sel == kMenu_Return)
			menu->sel = -1;
	}
	else if(input_ui_pressed(machine, IPT_UI_CANCEL))
	{
		menu->sel = -1;
	}

	return menu->sel + 1;
}

/*-----------------------------------------------------
  debug_cheat_menu - view internal cheat engine data
-----------------------------------------------------*/

static int debug_cheat_menu(running_machine *machine, cheat_menu_stack *menu)
{
	enum{
		kMenu_DriverName = 0,
		kMenu_GameName,
#ifdef MESS
		kMenu_CRC,
#endif
		kMenu_FullMenuPageHeight,
		kMenu_CheatListLength,
		kMenu_Separator,
		kMenu_CPURegionInfo,
		kMenu_KeyCodeChecker,
		kMenu_Return,
		kMenu_Max };

	UINT8		total		= 0;
	const char	** menuItem;
	const char	** menuSubItem;
	char		* driverNameBuf;
#ifdef MESS
	char		* crcBuf;
#endif
	char		* heightBuf;
	char		* numCodeBuf;

	/* first setting : NONE */
	if(menu->first_time)
		menu->first_time = 0;

	/* required items = (total items + return + terminator) & (strings buf * 3 [MAME]/4 [MESS]) */
	request_strings(kMenu_Max + 2, kMenu_Max, 64, 0);
	menuItem = menu_strings.main_list;
	menuSubItem = menu_strings.sub_list;
	driverNameBuf = menu_strings.main_strings[0];
	heightBuf = menu_strings.main_strings[1];
	numCodeBuf = menu_strings.main_strings[2];
#ifdef MESS
	crcBuf = menu_strings.main_strings[3];
#endif

	/********** MENU CONSTRUCTION **********/
	/* ##### DRIVER NAME ##### */
	menuItem[total] = "Driver";
	{
		astring * driverName = astring_alloc();

		sprintf(driverNameBuf, "%s", astring_c(core_filename_extract_base(driverName, machine->gamedrv->source_file, TRUE)));
		menuSubItem[total++] = driverNameBuf;
		astring_free(driverName);
	}

	/* ##### GAME / MACHINE NAME ##### */
#ifdef MESS
	menuItem[total]			= "Machine";
#else
	menuItem[total] 		= "Game";
#endif
	menuSubItem[total++]	= machine->gamedrv->name;

#ifdef MESS
	/* ##### CRC (MESS ONLY) ##### */
	menuItem[total] = "CRC";
	sprintf(crcBuf, "%X", thisGameCRC);
	menuSubItem[total++] = crcBuf;
#endif

	/* ##### FULL PAGE MENU HEIGHT ##### */
	menuItem[total] = "Default Menu Height";
	sprintf(heightBuf, "%d", fullMenuPageHeight);
	menuSubItem[total++] = heightBuf;

	/* ##### CHEAT LIST LENGTH ##### */
	menuItem[total] = "Total Code Entries";
	sprintf(numCodeBuf, "%d", cheatListLength);
	menuSubItem[total++] = numCodeBuf;

	/* ##### SEPARATOR ##### */
	menuItem[total] = MENU_SEPARATOR_ITEM;
	menuSubItem[total++] = NULL;

	/* ##### CPU/REGION INFO ##### */
	menuItem[total] = "View CPU/Region Info";
	menuSubItem[total++] = NULL;

	/* ##### ACTIVATION KEY CODE CHEAKCER ##### */
	menuItem[total] = "Check Activation Key Code";
	menuSubItem[total++] = NULL;

	/* ##### RETURN ##### */
	menuItem[total] = "Return to Prior Menu";
	menuSubItem[total++] = NULL;

	/* ##### TERMINATE ARRAY ##### */
	menuItem[total] = NULL;
	menuSubItem[total] = NULL;

	/* adjust cursor position */
	ADJUST_CURSOR(menu->sel, total)

	/* draw it */
	old_style_menu(menuItem, menuSubItem, NULL, menu->sel, 0);

	/********** KEY HANDLING **********/
	if(ui_pressed_repeat_throttle(machine, IPT_UI_UP, kVerticalKeyRepeatRate))
	{
		CURSOR_TO_PREVIOUS(menu->sel, total)

		if(menu->sel == kMenu_Separator)
			menu->sel--;
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_DOWN, kVerticalKeyRepeatRate))
	{
		CURSOR_TO_NEXT(menu->sel, total)

		if(menu->sel == kMenu_Separator)
			menu->sel++;
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_PAGE_UP, kVerticalKeyRepeatRate))
	{
		CURSOR_PAGE_UP(menu->sel)
	}
	else if(ui_pressed_repeat_throttle(machine, IPT_UI_PAGE_DOWN, kVerticalKeyRepeatRate))
	{
		CURSOR_PAGE_DOWN(menu->sel, total)
	}
	else if(input_ui_pressed(machine, IPT_UI_SELECT))
	{
		switch(menu->sel)
		{
			case kMenu_CPURegionInfo:
				cheat_menu_stack_push(view_cpu_region_info_list_menu, menu->handler, menu->sel);
				break;

			case kMenu_KeyCodeChecker:
				cheat_menu_stack_push(check_activation_key_code_menu, menu->handler, menu->sel);
				break;

			case kMenu_Return:
				menu->sel = -1;
				break;
		}
	}
	else if(input_ui_pressed(machine, IPT_UI_CANCEL))
		menu->sel = -1;

	return menu->sel + 1;
}
#endif
/*------------------
  cheate_periodic
------------------*/

static TIMER_CALLBACK( cheat_periodic )
{
	int i;

	if(input_ui_pressed(machine, IPT_UI_TOGGLE_CHEAT))
	{
		if(ShiftKeyPressed())
		{
			/* ##### WATCHPOINT ##### */
			watchesDisabled ^= 1;

			ui_popup_time(1, "Watchpoints %s", watchesDisabled ? "Off" : "On");
		}
		else
		{
			/* ##### CHEAT ##### */
			cheatsDisabled ^= 1;

			ui_popup_time(1, "Cheats %s", cheatsDisabled ? "Off" : "On");

			if(cheatsDisabled)
			{
				for(i = 0; i < cheatListLength; i++)
					TempDeactivateCheat(&cheatList[i]);
			}
		}
	}

	if(cheatsDisabled)
		return;

	for(i = 0; i < cheatListLength; i++)
		cheat_periodicEntry(machine, &cheatList[i]);

	if(EXTRACT_FIELD(cheatOptions, SearchBox) == kSearchBox_Minimum)
		SET_FIELD(cheatOptions, SearchBox, kSearchBox_Standard);
}

/*--------------
  PrintBinary
--------------*/

static UINT32 PrintBinary(char * buf, UINT32 data, UINT32 mask)
{
	UINT32	traverse = 0x80000000;
	UINT32	written = 0;

	while(traverse)
	{
		if(mask & traverse)
		{
			*buf++ = (data & traverse) ? '1' : '0';
			written++;
		}

		traverse >>= 1;
	}

	*buf++ = 0;

	return written;
}

/*-------------
  PrintASCII
-------------*/

static UINT32 PrintASCII(char * buf, UINT32 data, UINT8 size)
{
	switch(size)
	{
		case kSearchSize_8Bit:
		case kSearchSize_1Bit:
		default:
			buf[0] = (data >> 0) & 0xFF;
			buf[1] = 0;

			return 1;

		case kSearchSize_16Bit:
			buf[0] = (data >> 8) & 0xFF;
			buf[1] = (data >> 0) & 0xFF;
			buf[2] = 0;

			return 2;

		case kSearchSize_24Bit:
			buf[0] = (data >> 16)& 0xFF;
			buf[1] = (data >> 8) & 0xFF;
			buf[2] = (data >> 0) & 0xFF;
			buf[3] = 0;

			return 3;

		case kSearchSize_32Bit:
			buf[0] = (data >> 24) & 0xFF;
			buf[1] = (data >> 16) & 0xFF;
			buf[2] = (data >>  8) & 0xFF;
			buf[3] = (data >>  0) & 0xFF;
			buf[4] = 0;

			return 4;
	}
}

/*---------------------------------------------
  cheat_display_watches - display watchpoint
---------------------------------------------*/

void cheat_display_watches(void)
{
	int i;

	if(watchesDisabled)
		return;

	for(i = 0; i < watchListLength; i++)
	{
		WatchInfo * info = &watchList[i];

		int			j;
		int			numChars;
		int			xOffset			= 0;
		int			yOffset			= 0;
		int			lineElements	= 0;
		UINT32		address			= info->address;

		char		buf[1024];

		if(info->numElements)
		{
			/* ##### LABEL ##### */
			switch(info->labelType)
			{
				case kWatchLabel_Address:
					numChars = sprintf(buf, "%.8X: ", info->address);

					ui_draw_text(buf, xOffset * ui_get_char_width('0') + info->x, yOffset * ui_get_line_height() + info->y);
					xOffset += numChars;
					break;

				case kWatchLabel_String:
					numChars = sprintf(buf, "%s: ", info->label);

					ui_draw_text(buf, xOffset * ui_get_char_width('0') + info->x, yOffset * ui_get_line_height() + info->y);
					xOffset += numChars;
					break;
			}

			/* ##### VALUE ##### */
			for(j = 0; j < info->numElements; j++)
			{
				UINT32 data = 0;

				if(!TEST_FIELD(info->cpu, RegionFlag))
				{
					UINT8 cpu = EXTRACT_FIELD(info->cpu, CPUIndex);

					switch(EXTRACT_FIELD(info->cpu, AddressSpace))
					{
						case kAddressSpace_Program:
							data = (DoCPURead(cpu, address, kSearchByteIncrementTable[info->elementBytes], cpu_needs_swap(cpu)) + info->addValue) & kSearchByteMaskTable[info->elementBytes];
							break;

						case kAddressSpace_DataSpace:
						case kAddressSpace_IOSpace:
						case kAddressSpace_OpcodeRAM:
						{
							UINT8 * buf = (UINT8 *) GetMemoryRegionBasePointer(cpu, EXTRACT_FIELD(info->cpu, AddressSpace), address);

							if(buf)
								data =	DoMemoryRead(buf, address, kSearchByteIncrementTable[info->elementBytes], cpu_needs_swap(cpu) + info->addValue, get_cpu_info(cpu)) &
										kSearchByteMaskTable[info->elementBytes];
						}
						break;

						default:
							return;
					}
				}
				else
				{
					UINT8 * buf = memory_region(info->cpu);

					if(buf)
						data =	DoMemoryRead(buf, address, kSearchByteIncrementTable[info->elementBytes],
								region_needs_swap(info->cpu) + info->addValue,
								get_region_info(info->cpu)) & kSearchByteMaskTable[info->elementBytes];
				}

				data = DoShift(data, info->dataShift);
				data ^= info->xor;

				if(	(lineElements >= info->elementsPerLine) && info->elementsPerLine)
				{
					lineElements = 0;
					xOffset = 0;
					yOffset++;
				}

				switch(info->displayType)
				{
					case kWatchDisplayType_Hex:
						numChars = sprintf(buf, "%.*X", kSearchByteDigitsTable[info->elementBytes], data);

						ui_draw_text(buf, xOffset * ui_get_char_width('0') + info->x, yOffset * ui_get_line_height() + info->y);
						xOffset += numChars;
						xOffset++;
						break;

					case kWatchDisplayType_Decimal:
						numChars = sprintf(buf, "%.*d", kSearchByteDecDigitsTable[info->elementBytes], data);

						ui_draw_text(buf, xOffset * ui_get_char_width('0') + info->x, yOffset * ui_get_line_height() + info->y);
						xOffset += numChars;
						xOffset++;
						break;

					case kWatchDisplayType_Binary:
						numChars = PrintBinary(buf, data, kSearchByteMaskTable[info->elementBytes]);

						ui_draw_text(buf, xOffset * ui_get_char_width('0') + info->x, yOffset * ui_get_line_height() + info->y);
						xOffset += numChars;
						xOffset++;
						break;

					case kWatchDisplayType_ASCII:
						numChars = PrintASCII(buf, data, info->elementBytes);

						ui_draw_text(buf, xOffset * ui_get_char_width('0') + info->x, yOffset * ui_get_line_height() + info->y);
						xOffset += numChars;
						break;
				}

				address += kSearchByteIncrementTable[info->elementBytes] + info->skip;
				lineElements++;
			}
		}
	}
}

/*------------------
  ResizeCheatList
------------------*/

static void ResizeCheatList(UINT32 newLength)
{
	if(newLength != cheatListLength)
	{
		if(newLength < cheatListLength)
		{
			int	i;

			for(i = newLength; i < cheatListLength; i++)
				DisposeCheat(&cheatList[i]);
		}

		cheatList = realloc(cheatList, newLength * sizeof(CheatEntry));

		if(!cheatList && (newLength != 0))
		{
			logerror("ResizeCheatList: out of memory resizing cheat list\n");
			ui_popup_time(2, "out of memory while loading cheat database");

			cheatListLength = 0;

			return;
		}

		if(newLength > cheatListLength)
		{
			int	i;

			memset(&cheatList[cheatListLength], 0, (newLength - cheatListLength) * sizeof(CheatEntry));

			for(i = cheatListLength; i < newLength; i++)
				cheatList[i].flags |= kCheatFlag_Dirty;
		}

		cheatListLength = newLength;
	}
}

/*---------------------------
  ResizeCheatListNoDispose
---------------------------*/

static void ResizeCheatListNoDispose(UINT32 newLength)
{
	if(newLength != cheatListLength)
	{
		cheatList = realloc(cheatList, newLength * sizeof(CheatEntry));

		if(!cheatList && (newLength != 0))
		{
			logerror("ResizeCheatListNoDispose: out of memory resizing cheat list\n");
			ui_popup_time(2, "out of memory while loading cheat database");

			cheatListLength = 0;

			return;
		}

		if(newLength > cheatListLength)
		{
			int	i;

			memset(&cheatList[cheatListLength], 0, (newLength - cheatListLength) * sizeof(CheatEntry));

			for(i = cheatListLength; i < newLength; i++)
				cheatList[i].flags |= kCheatFlag_Dirty;
		}

		cheatListLength = newLength;
	}
}

/*---------------------------------------------------------------
  AddCheatBefore - insert new cheat entry before selected code
---------------------------------------------------------------*/

static void AddCheatBefore(UINT32 idx)
{
	ResizeCheatList(cheatListLength + 1);

	if(idx < (cheatListLength - 1))
		memmove(&cheatList[idx + 1], &cheatList[idx], sizeof(CheatEntry) * (cheatListLength - 1 - idx));

	if(idx >= cheatListLength)
		idx = cheatListLength - 1;

	memset(&cheatList[idx], 0, sizeof(CheatEntry));

	cheatList[idx].flags |= kCheatFlag_Dirty;

	ResizeCheatActionList(&cheatList[idx], 1);

	cheatList[idx].actionList[0].extendData = ~0;
	cheatList[idx].actionList[0].lastValue = NULL;

	messageType = kCheatMessage_SucceededToAdd;
	messageTimer = DEFAULT_MESSAGE_TIME;
}

/*----------------
  DeleteCheatAt
----------------*/

static void DeleteCheatAt(UINT32 idx)
{
	if(idx >= cheatListLength)
	{
		messageType = kCheatMessage_FailedToDelete;
		messageTimer = DEFAULT_MESSAGE_TIME;
		return;
	}

	DisposeCheat(&cheatList[idx]);

	if(idx < (cheatListLength - 1))
	{
		memmove(&cheatList[idx], &cheatList[idx + 1], sizeof(CheatEntry) * (cheatListLength - 1 - idx));
	}

	ResizeCheatListNoDispose(cheatListLength - 1);

	messageType = kCheatMessage_SucceededToDelete;
	messageTimer = DEFAULT_MESSAGE_TIME;
}

/*------------------------------------------------
  DisposeCheat - free memory for selected entry
------------------------------------------------*/

static void DisposeCheat(CheatEntry * entry)
{
	if(entry)
	{
		int	i;

		free(entry->name);
		free(entry->comment);
		free(entry->labelIndex);

		for(i = 0; i < entry->actionListLength; i++)
		{
			CheatAction	* action = &entry->actionList[i];

			DisposeAction(action);
		}

		free(entry->actionList);

		memset(entry, 0, sizeof(CheatEntry));
	}
}

/*-----------------------------------------------------
  GetNewCheat - insert new cheat entry and return it
-----------------------------------------------------*/

static CheatEntry *	GetNewCheat(void)
{
	AddCheatBefore(cheatListLength);

	return &cheatList[cheatListLength - 1];
}

/*------------------------
  ResizeCheatActionList
------------------------*/

static void ResizeCheatActionList(CheatEntry * entry, UINT32 newLength)
{
	if(newLength != entry->actionListLength)
	{
		if(newLength < entry->actionListLength)
		{
			int	i;

			for(i = newLength; i < entry->actionListLength; i++)
				DisposeAction(&entry->actionList[i]);
		}

		entry->actionList = realloc(entry->actionList, newLength * sizeof(CheatAction));
		if(!entry->actionList && (newLength != 0))
		{
			logerror("ResizeCheatActionList: out of memory resizing cheat action list\n");
			ui_popup_time(2, "out of memory while loading cheat database");

			entry->actionListLength = 0;

			return;
		}

		if(newLength > entry->actionListLength)
		{
			memset(&entry->actionList[entry->actionListLength], 0, (newLength - entry->actionListLength) * sizeof(CheatAction));
		}

		entry->actionListLength = newLength;
	}
}

/*---------------------------------
  ResizeCheatActionListNoDispose
---------------------------------*/
#if 0
static void ResizeCheatActionListNoDispose(CheatEntry * entry, UINT32 newLength)
{
	if(newLength != entry->actionListLength)
	{
		entry->actionList = realloc(entry->actionList, newLength * sizeof(CheatAction));
		if(!entry->actionList && (newLength != 0))
		{
			logerror("ResizeCheatActionList: out of memory resizing cheat action list\n");
			ui_popup_time(2, "out of memory while loading cheat database");

			entry->actionListLength = 0;

			return;
		}

		if(newLength > entry->actionListLength)
		{
			memset(&entry->actionList[entry->actionListLength], 0, (newLength - entry->actionListLength) * sizeof(CheatAction));
		}

		entry->actionListLength = newLength;
	}
}

/*----------------------------------------------------------------
  AddActionBefore - Insert empty Action
                    This function is only called in EditCheat()
----------------------------------------------------------------*/

static void AddActionBefore(CheatEntry * entry, UINT32 idx)
{
	ResizeCheatActionList(entry, entry->actionListLength + 1);

	if(idx < (entry->actionListLength - 1))
		memmove(&entry->actionList[idx + 1], &entry->actionList[idx], sizeof(CheatAction) * (entry->actionListLength - 1 - idx));

	if(idx >= entry->actionListLength)
		idx = entry->actionListLength - 1;

	memset(&entry->actionList[idx], 0, sizeof(CheatAction));

	cheatList[idx].actionList[0].extendData = ~0;
	cheatList[idx].actionList[0].lastValue = NULL;
}

/*-----------------
  DeleteActionAt
-----------------*/

static void DeleteActionAt(CheatEntry * entry, UINT32 idx)
{
	if(idx >= entry->actionListLength)
	{
		messageType = kCheatMessage_FailedToDelete;
		messageTimer = DEFAULT_MESSAGE_TIME;
		return;
	}

	DisposeAction(&entry->actionList[idx]);

	if(idx < (entry->actionListLength - 1))
		memmove(&entry->actionList[idx], &entry->actionList[idx + 1], sizeof(CheatAction) * (entry->actionListLength - 1 - idx));

	ResizeCheatActionListNoDispose(entry, entry->actionListLength - 1);

	messageTimer = DEFAULT_MESSAGE_TIME;
	messageType = kCheatMessage_SucceededToDelete;
}
#endif
/*----------------
  DisposeAction
----------------*/

static void DisposeAction(CheatAction * action)
{
	if(action)
	{
		free(action->optionalName);

		if(action->lastValue)
			free(action->lastValue);

		memset(action, 0, sizeof(CheatAction));
	}
}

/*------------------------------------------------
  InitWatch - initialize a value for watchpoint
------------------------------------------------*/

static void InitWatch(WatchInfo * info, UINT32 idx)
{
	/* NOTE : 1st watchpoint should be always Y = 0 */
	if(idx > 0)
		info->y = watchList[idx - 1].y + ui_get_line_height();
	else
		info->y = 0;
}

/*------------------
  ResizeWatchList
------------------*/

static void ResizeWatchList(UINT32 newLength)
{
	if(newLength != watchListLength)
	{
		if(newLength < watchListLength)
		{
			int	i;

			for(i = newLength; i < watchListLength; i++)
				DisposeWatch(&watchList[i]);
		}

		watchList = realloc(watchList, newLength * sizeof(WatchInfo));
		if(!watchList && (newLength != 0))
		{
			logerror("ResizeWatchList: out of memory resizing watch list\n");
			ui_popup_time(2, "out of memory while adding watch");

			watchListLength = 0;

			return;
		}

		if(newLength > watchListLength)
		{
			int	i;

			memset(&watchList[watchListLength], 0, (newLength - watchListLength) * sizeof(WatchInfo));

			for(i = watchListLength; i < newLength; i++)
				InitWatch(&watchList[i], i);
		}

		watchListLength = newLength;
	}
}

/*---------------------------
  ResizeWatchListNoDispose
---------------------------*/

static void ResizeWatchListNoDispose(UINT32 newLength)
{
	if(newLength != watchListLength)
	{
		watchList = realloc(watchList, newLength * sizeof(WatchInfo));
		if(!watchList && (newLength != 0))
		{
			logerror("ResizeWatchList: out of memory resizing watch list\n");
			ui_popup_time(2, "out of memory while adding watch");

			watchListLength = 0;

			return;
		}

		if(newLength > watchListLength)
		{
			int	i;

			memset(&watchList[watchListLength], 0, (newLength - watchListLength) * sizeof(WatchInfo));

			for(i = watchListLength; i < newLength; i++)
				InitWatch(&watchList[i], i);
		}

		watchListLength = newLength;
	}
}

/*-----------------
  AddWatchBefore
-----------------*/

static void AddWatchBefore(UINT32 idx)
{
	ResizeWatchList(watchListLength + 1);

	if(idx < (watchListLength - 1))
		memmove(&watchList[idx + 1], &watchList[idx], sizeof(WatchInfo) * (watchListLength - 1 - idx));

	if(idx >= watchListLength)
		idx = watchListLength - 1;

	memset(&watchList[idx], 0, sizeof(WatchInfo));

	InitWatch(&watchList[idx], idx);
}

/*----------------
  DeleteWatchAt
----------------*/

static void DeleteWatchAt(UINT32 idx)
{
	if(idx >= watchListLength)
		return;

	DisposeWatch(&watchList[idx]);

	if(idx < (watchListLength - 1))
		memmove(&watchList[idx], &watchList[idx + 1], sizeof(WatchInfo) * (watchListLength - 1 - idx));

	ResizeWatchListNoDispose(watchListLength - 1);
}

/*---------------
  DisposeWatch
---------------*/

static void DisposeWatch(WatchInfo * watch)
{
	if(watch)
		memset(watch, 0, sizeof(WatchInfo));
}

/*----------------------------------------------------------
  GetUnusedWatch - search unused watchpoint and return it
----------------------------------------------------------*/

static WatchInfo * GetUnusedWatch(void)
{
	int			i;
	WatchInfo	* info;
	WatchInfo	* theWatch = NULL;

	/* search unused watchpoint */
	for(i = 0; i < watchListLength; i++)
	{
		info = &watchList[i];

		if(info->numElements == 0)
		{
			theWatch = info;
			break;
		}
	}

	/* if unused watchpoint is not found, insert new watchpoint */
	if(!theWatch)
	{
		AddWatchBefore(watchListLength);

		theWatch = &watchList[watchListLength - 1];
	}

	return theWatch;
}

/*----------------------------------------------------------------------------
  AddCheatFromWatch - add new cheat code from watchpoint list to cheat list
----------------------------------------------------------------------------*/

static void AddCheatFromWatch(WatchInfo * watch)
{
	if(watch)
	{
		int			tempStringLength;
		char		tempString[1024];
		CheatEntry	* entry				= GetNewCheat();
		CheatAction	* action			= &entry->actionList[0];

		/* set parameters */
		action->region		= watch->cpu;
		SET_FIELD(action->type, CPURegion, action->region);
		SET_FIELD(action->type, AddressSize, watch->elementBytes);
		action->address		= action->originalAddress	= watch->address;
		action->extendData	= ~0;
		action->lastValue	= NULL;

		action->data 		= ReadData(action);

		/* set name */
		tempStringLength = sprintf(tempString, "%.8X (%d) = %.*X", watch->address, watch->cpu, kSearchByteDigitsTable[watch->elementBytes], action->data);
		entry->name = create_string_copy(tempString);

		UpdateCheatInfo(entry, 0);

		messageType		= kCheatMessage_SucceededToAdd;
		messageTimer	= DEFAULT_MESSAGE_TIME;
	}
	else
	{
		messageType		= kCheatMessage_FailedToAdd;
		messageTimer	= DEFAULT_MESSAGE_TIME;
	}
}

/*--------------------------------------------------------------------------------------
  SetupCheatFromWatchAsWatch - add new watch code from watchpoint list to cheat list
                               If watchpoint is not displayed (length = 0), don't add
--------------------------------------------------------------------------------------*/

static void SetupCheatFromWatchAsWatch(CheatEntry * entry, WatchInfo * watch)
{
	if(watch && entry && watch->numElements)
	{
		char			tempString[256];
		CheatAction		* action;

		DisposeCheat(entry);
		ResizeCheatActionList(entry, 1);

		action = &entry->actionList[0];

		sprintf(tempString, "Watch %.8X (%d)", watch->address, watch->cpu);

		/* set name */
		entry->name		= create_string_copy(tempString);
		entry->comment	= create_string_copy(watch->label);

		/* set parameters */
		SET_FIELD(action->type, CodeType, kCodeType_Watch);
		SET_FIELD(action->type, AddressSize, kSearchByteIncrementTable[watch->elementBytes] - 1);

		action->region				=	watch->cpu;
		action->address				=	watch->address;
		action->originalAddress		=	action->address;
		action->originalDataField	=	action->data;
		action->extendData			=	0xFFFFFFFF;

		SET_FIELD(action->data, WatchNumElements, watch->numElements - 1);
		SET_FIELD(action->data, WatchSkip, watch->skip);
		SET_FIELD(action->data, WatchElementsPerLine, watch->elementsPerLine);
		SET_FIELD(action->data, WatchAddValue, watch->addValue);

		/* set comment */

		UpdateCheatInfo(entry, 0);

		messageType		= kCheatMessage_SucceededToAdd;
		messageTimer	= DEFAULT_MESSAGE_TIME;
	}
	else
	{
		messageType		= kCheatMessage_FailedToAdd;
		messageTimer	= DEFAULT_MESSAGE_TIME;
	}
}

/*-----------------------------------------------------------
  ResetWatch - clear data in WatchInfo except x/y position
-----------------------------------------------------------*/

static void ResetWatch(WatchInfo * watch)
{
	if(watch)
	{
		watch->address			= 0;
		watch->cpu				= 0;
		watch->numElements		= 0;
		watch->elementBytes		= kWatchSizeConversionTable[0];
		watch->labelType		= kWatchLabel_None;
		watch->displayType		= kWatchDisplayType_Hex;
		watch->skip				= 0;
		watch->elementsPerLine	= 0;
		watch->addValue			= 0;
		watch->addressShift		= 0;
		watch->dataShift		= 0;
		watch->xor				= 0;
		watch->linkedCheat		= NULL;
		watch->label[0]			= 0;
	}
}

/*------------------------------------------------------------------
  resize_search_list - reallocate search list if different length
------------------------------------------------------------------*/

static void resize_search_list(UINT32 new_length)
{
	if(new_length != search_list_length)
	{
		/* if delete, dispose deleted or later search lists */
		if(new_length < search_list_length)
		{
			int i;

			for(i = new_length; i < search_list_length; i++)
				dispose_search(&search_list[i]);
		}

		/* reallocate search list */
		search_list = realloc(search_list, new_length * sizeof(search_info));

		if(search_list == NULL && new_length != 0)
		{
			/* memory allocation error */
			logerror("ResizeSearchList: out of memory resizing search list\n");
			ui_popup_time(2, "out of memory while adding search");

			search_list_length = 0;

			return;
		}

		/* if insert, initialize new search list */
		if(new_length > search_list_length)
		{
			int i;

			memset(&search_list[search_list_length], 0, (new_length - search_list_length) * sizeof(search_info));

			for(i = search_list_length; i < new_length; i++)
				init_search(&search_list[i]);
		}

		search_list_length = new_length;
	}
}

/*--------------------------------
  resize_search_list_no_dispose
--------------------------------*/
#if 0
static void ResizeSearchListNoDispose(UINT32 newLength)
{
	if(newLength != search_list_length)
	{
		searchList = realloc(searchList, newLength * sizeof(search_info));

		if(!searchList && (newLength != 0))
		{
			logerror("ResizeSearchList: out of memory resizing search list\n");
			ui_popup_time(2, "out of memory while adding search");

			search_list_length = 0;

			return;
		}

		if(newLength > search_list_length)
			memset(&searchList[search_list_length], 0, (newLength - search_list_length) * sizeof(search_info));

		search_list_length = newLength;
	}
}

/*--------------------
  add_search_before
--------------------*/

static void AddSearchBefore(UINT32 idx)
{
	ResizeSearchListNoDispose(search_list_length + 1);

	if(idx < (search_list_length - 1))
		memmove(&search_list[idx + 1], &search_list[idx], sizeof(search_info) * (search_list_length - 1 - idx));

	if(idx >= search_list_length)
		idx = search_list_length - 1;

	memset(&search_list[idx], 0, sizeof(search_info));

	init_search(&search_list[idx]);
}

/*-------------------
  delete_search_at
-------------------*/

static void DeleteSearchAt(UINT32 idx)
{
	if(idx >= search_list_length)
		return;

	DisposeSearch(idx);

	if(idx < (search_list_length - 1))
		memmove(&search_list[idx], &search_list[idx + 1], sizeof(search_info) * (search_list_length - 1 - idx));

	ResizeSearchListNoDispose(search_list_length - 1);
}
#endif
/*---------------------------------------
  init_search - initialize search info
---------------------------------------*/

static void init_search(search_info *info)
{
	if(info)
		info->search_speed = ksearch_speed_Medium;
}

/*---------------------------------------------------------------------------
  dispose_search_reigons - free all search regions in selected search info
---------------------------------------------------------------------------*/

static void dispose_search_reigons(search_info *info)
{
	if(info->region_list)
	{
		int i;

		for(i = 0; i < info->region_list_length; i++)
		{
			search_region *region = &info->region_list[i];

			free(region->first);
			free(region->last);
			free(region->status);
			free(region->backup_last);
			free(region->backup_status);
		}

		free(info->region_list);

		info->region_list = NULL;
	}

	info->region_list_length = 0;
}

/*-------------------------------------------------------------------------
  dispose_search - free selected search info and all search region in it
-------------------------------------------------------------------------*/

static void dispose_search(search_info *info)
{
	dispose_search_reigons(info);

	free(info->name);

	info->name = NULL;
}

/*--------------------------------------------------
  get_current_search - return working search info
--------------------------------------------------*/

static search_info *get_current_search(void)
{
	if(current_search_idx >= search_list_length)
		current_search_idx = search_list_length - 1;

	if(current_search_idx < 0)
		current_search_idx = 0;

	return &search_list[current_search_idx];
}

/*----------------------------------------------------------------------------------------------
  fill_buffer_from_region - fill selected search region with a value read from search address
----------------------------------------------------------------------------------------------*/

static void fill_buffer_from_region(search_region *region, UINT8 * buf)
{
	UINT32 offset;

	/* ##### optimize if needed ##### */
	for(offset = 0; offset < region->length; offset++)
		buf[offset] = read_region_data(region, offset, 1, 0);
}

/*---------------------------------------------------------------------------
  read_region_data - read a data from memory or search region in searching
---------------------------------------------------------------------------*/

static UINT32 read_region_data(search_region *region, UINT32 offset, UINT8 size, UINT8 swap)
{
	UINT32 address = region->address + offset;

	switch(region->target_type)
	{
		case kRegionType_CPU:
			return DoCPURead(region->target_idx, address, size, cpu_needs_swap(region->target_idx) ^ swap);

		case kRegionType_Memory:
			{
				UINT8 * buf = (UINT8 *)region->cached_pointer;

				if(buf)
/*                  return DoMemoryRead(region->cached_pointer, address, size, swap, &raw_cpu_info); */
					return DoMemoryRead(buf, address, size, cpu_needs_swap(region->target_idx) ^ swap, get_cpu_info(region->target_idx));
				else
					return 0;
			}
	}

	return 0;
}
/*------------------------------------------------
  backup_search - back up current search region
------------------------------------------------*/

static void backup_search(search_info *info)
{
	int i;

	for(i = 0; i < info->region_list_length; i++)
		backup_region(&info->region_list[i]);

	info->old_num_results = info->num_results;
	info->backup_valid = 1;
}

/*---------------------------------------------------------
  restore_search_backup - restore previous search region
---------------------------------------------------------*/

static void restore_search_backup(search_info *info)
{
	int i;

	if(info && info->backup_valid)
	{
		for(i = 0; i < info->region_list_length; i++)
			restore_region_backup(&info->region_list[i]);

		info->num_results	= info->old_num_results;
		info->backup_valid	= 0;

#if 1
		messageTimer = DEFAULT_MESSAGE_TIME;
		messageType = kCheatMessage_RestoreValue;
#else
		ui_popup_time(1, "values restored");	/* not displayed when ui is opend */
#endif
	}
	else
	{
#if 1
		messageTimer = DEFAULT_MESSAGE_TIME;
		messageType = kCheatMessage_NoOldValue;
#else
		ui_popup_time(1, "there are no old values");	/* not displayed when ui is opend */
#endif
	}
}

/*-------------------------------------------------
  backup_region - back up current search results
-------------------------------------------------*/

static void backup_region(search_region *region)
{
	if(region->flags & kRegionFlag_Enabled)
	{
		memcpy(region->backup_last,		region->last,	region->length);
		memcpy(region->backup_status,	region->status,	region->length);

		region->old_num_results = region->num_results;
	}
}

/*----------------------------------------------------------
  restore_region_backup - restore previous search results
----------------------------------------------------------*/

static void restore_region_backup(search_region *region)
{
	if(region->flags & kRegionFlag_Enabled)
	{
		memcpy(region->last,	region->backup_last,	region->length);
		memcpy(region->status,	region->backup_status,	region->length);

		region->num_results = region->old_num_results;
	}
}

/*--------------------------------------------------------
  default_enable_region - get default regions to search
--------------------------------------------------------*/

static UINT8 default_enable_region(running_machine *machine, search_region *region, search_info *info)
{
	write8_machine_func	handler			= region->write_handler->write.mhandler8;
	FPTR				handlerAddress	= (FPTR)handler;

	switch(info->search_speed)
	{
		case ksearch_speed_Fast:
#if HAS_SH2
			if(	machine->config->cpu[0].type == CPU_SH2 &&
				(info->target_type == kRegionType_CPU && info->target_idx == 0 && region->address == 0x06000000))
					return 1;
			else
				return 0;
#endif
			if(handler == SMH_RAM && region->write_handler->baseptr != NULL)
				return 1;

		case ksearch_speed_Medium:
			if(handlerAddress >= (FPTR)SMH_BANK1 && handlerAddress <= (FPTR)SMH_BANK24)
				return 1;

			if(handler == SMH_RAM)
				return 1;

			return 0;

		case ksearch_speed_Slow:
			if(handler == SMH_NOP || handler == SMH_ROM)
				return 0;

			if(handlerAddress > STATIC_COUNT && region->write_handler->baseptr == NULL)
				return 0;

			return 1;

		case ksearch_speed_VerySlow:
			if(handler == SMH_NOP || handler == SMH_ROM)
				return 0;

			return 1;
	}

	return 0;
}

/*---------------------------------------------------------------------
  set_search_region_default_name - set name of region to region list
---------------------------------------------------------------------*/

static void set_search_region_default_name(search_region *region)
{
	switch(region->target_type)
	{
		case kRegionType_CPU:
		{
			char desc[16];

			if(region->write_handler)
			{
				genf	*handler		= region->write_handler->write.generic;
				FPTR	handler_address	= (FPTR)handler;

				if(handler_address >= (FPTR)SMH_BANK1 && handler_address <= (FPTR)SMH_BANK24)
				{
					sprintf(desc, "BANK%.2d", (int)(handler_address - (FPTR)SMH_BANK1) + 1);
				}
				else
				{
					switch(handler_address)
					{
						case (FPTR)SMH_NOP:		strcpy(desc, "NOP   ");	break;
						case (FPTR)SMH_RAM:		strcpy(desc, "RAM   ");	break;
						case (FPTR)SMH_ROM:		strcpy(desc, "ROM   ");	break;
						default:				strcpy(desc, "CUSTOM");	break;
					}
				}
			}
			else
				sprintf(desc, "CPU%.2d ", region->target_idx);

			sprintf(region->name,	"%.*X-%.*X %s",
									cpu_info_list[region->target_idx].address_chars_needed, region->address,
									cpu_info_list[region->target_idx].address_chars_needed, region->address + region->length - 1,
									desc);
		}
		break;

		case kRegionType_Memory:	/* unused? */
			sprintf(region->name, "%.8X-%.8X MEMORY", region->address, region->address + region->length - 1);
			break;

		default:
			sprintf(region->name, "UNKNOWN");
	}
}

/*-------------------------------------------------------------------------------------
  allocate_search_regions - free memory for search region then realloc if searchable
-------------------------------------------------------------------------------------*/

static void allocate_search_regions(search_info *info)
{
	int i;

	info->backup_valid	= 0;
	info->num_results	= 0;

	for(i = 0; i < info->region_list_length; i++)
	{
		search_region *region;

		region				= &info->region_list[i];
		region->num_results	= 0;

		free(region->first);
		free(region->last);
		free(region->status);
		free(region->backup_last);
		free(region->backup_status);

		if(region->flags & kRegionFlag_Enabled)
		{
			region->first			= malloc(region->length);
			region->last			= malloc(region->length);
			region->status			= malloc(region->length);
			region->backup_last		= malloc(region->length);
			region->backup_status	= malloc(region->length);

			if(region->first == NULL || region->last == NULL || region->status == NULL || region->backup_last == NULL || region->backup_status == NULL)
				fatalerror(	"cheat: [search region] memory allocation error\n"
							"	----- %s -----\n"
							"	first			= %p\n"
							"	last			= %p\n"
							"	status			= %p\n"
							"	backup_last		= %p\n"
							"	backup_status	= %p\n",
							region->name,
							region->first, region->last, region->status, region->backup_last, region->backup_status);
		}
		else
		{
			region->first			= NULL;
			region->last			= NULL;
			region->status			= NULL;
			region->backup_last		= NULL;
			region->backup_status	= NULL;
		}
	}
}

/*--------------------------------------------------------------
  build_search_regions - build serach regions from memory map
--------------------------------------------------------------*/

static void build_search_regions(running_machine *machine, search_info *info)
{
	info->comparison = kSearchComparison_EqualTo;

	/* 1st, free all search regions in current search_info */
	dispose_search_reigons(info);

	/* 2nd, set parameters from memory map */
	switch(info->target_type)
	{
		case kRegionType_CPU:
		{
			if(info->search_speed == ksearch_speed_AllMemory)
			{
				UINT32			length = cpu_info_list[info->target_idx].address_mask + 1;
				search_region	*region;

				info->region_list			= calloc(sizeof(search_region), 1);
				info->region_list_length	= 1;

				region					= info->region_list;
				region->address			= 0;
				region->length			= length;
				region->target_idx		= info->target_idx;
				region->target_type		= info->target_type;
				region->write_handler	= NULL;
				region->first			= NULL;
				region->last			= NULL;
				region->status			= NULL;
				region->backup_last		= NULL;
				region->backup_status	= NULL;
				region->flags			= kRegionFlag_Enabled;

				set_search_region_default_name(region);
			}
			else
			{
				if(VALID_CPU(info->target_idx))
				{
					int							count = 0;
					const address_map			*map = NULL;
					const address_map_entry		*entry;
					search_region				*traverse;

					map = memory_get_address_map(info->target_idx, ADDRESS_SPACE_PROGRAM);

					for(entry = map->entrylist; entry != NULL; entry = entry->next)
						if(entry->write.generic)
							count++;

					info->region_list			= calloc(sizeof(search_region), count);
					info->region_list_length	= count;
					traverse					= info->region_list;

					for(entry = map->entrylist; entry != NULL; entry = entry->next)
					{
						if (entry->write.generic)
						{
							UINT32 length = (entry->addrend - entry->addrstart) + 1;

							traverse->address		= entry->addrstart;
							traverse->length		= length;
							traverse->target_idx	= info->target_idx;
							traverse->target_type	= info->target_type;
							traverse->write_handler	= entry;
							traverse->first			= NULL;
							traverse->last			= NULL;
							traverse->status		= NULL;
							traverse->backup_last	= NULL;
							traverse->backup_status	= NULL;
							traverse->flags			= default_enable_region(machine, traverse, info) ? kRegionFlag_Enabled : 0;

							set_search_region_default_name(traverse);

							traverse++;
						}
					}
				}
			}
		}
		break;

		case kRegionType_Memory:	/* unused? */
			break;
	}
}

/*---------------------------------------------------------------------
  ConvertOldCode - convert old format code to new when load database
---------------------------------------------------------------------*/

static int ConvertOldCode(int code, int cpu, int * data, int * extendData)
{
	enum
	{
		kCustomField_None =					0,
		kCustomField_DontApplyCPUField =	1 << 0,
		kCustomField_SetBit =				1 << 1,
		kCustomField_ClearBit =				1 << 2,
		kCustomField_SubtractOne =			1 << 3,

		kCustomField_BitMask =				kCustomField_SetBit |
											kCustomField_ClearBit,
		kCustomField_End =					0xFF
	};

	struct ConversionTable
	{
		int		oldCode;
		UINT32	newCode;
		UINT8	customField;
	};

	static const struct ConversionTable kConversionTable[] =
	{
		{	0,		0x00000000,	kCustomField_None },
		{	1,		0x00000001,	kCustomField_None },
		{	2,		0x00000020,	kCustomField_None },
		{	3,		0x00000040,	kCustomField_None },
		{	4,		0x000000A0,	kCustomField_None },
		{	5,		0x00000022,	kCustomField_None },
		{	6,		0x00000042,	kCustomField_None },
		{	7,		0x000000A2,	kCustomField_None },
		{	8,		0x00000024,	kCustomField_None },
		{	9,		0x00000044,	kCustomField_None },
		{	10,		0x00000064,	kCustomField_None },
		{	11,		0x00000084,	kCustomField_None },
		{	15,		0x00000023,	kCustomField_None },
		{	16,		0x00000043,	kCustomField_None },
		{	17,		0x000000A3,	kCustomField_None },
		{	20,		0x00000000,	kCustomField_SetBit },
		{	21,		0x00000001,	kCustomField_SetBit },
		{	22,		0x00000020,	kCustomField_SetBit },
		{	23,		0x00000040,	kCustomField_SetBit },
		{	24,		0x000000A0,	kCustomField_SetBit },
		{	40,		0x00000000,	kCustomField_ClearBit },
		{	41,		0x00000001,	kCustomField_ClearBit },
		{	42,		0x00000020,	kCustomField_ClearBit },
		{	43,		0x00000040,	kCustomField_ClearBit },
		{	44,		0x000000A0,	kCustomField_ClearBit },
		{	60,		0x00000103,	kCustomField_None },
		{	61,		0x00000303,	kCustomField_None },
		{	62,		0x00000503,	kCustomField_SubtractOne },
		{	63,		0x00000903,	kCustomField_None },
		{	64,		0x00000B03,	kCustomField_None },
		{	65,		0x00000D03,	kCustomField_SubtractOne },
		{	70,		0x00000101,	kCustomField_None },
		{	71,		0x00000301,	kCustomField_None },
		{	72,		0x00000501,	kCustomField_SubtractOne },
		{	73,		0x00000901,	kCustomField_None },
		{	74,		0x00000B01,	kCustomField_None },
		{	75,		0x00000D01,	kCustomField_SubtractOne },
		{	80,		0x00000102,	kCustomField_None },
		{	81,		0x00000302,	kCustomField_None },
		{	82,		0x00000502,	kCustomField_SubtractOne },
		{	83,		0x00000902,	kCustomField_None },
		{	84,		0x00000B02,	kCustomField_None },
		{	85,		0x00000D02,	kCustomField_SubtractOne },
		{	90,		0x00000100,	kCustomField_None },
		{	91,		0x00000300,	kCustomField_None },
		{	92,		0x00000500,	kCustomField_SubtractOne },
		{	93,		0x00000900,	kCustomField_None },
		{	94,		0x00000B00,	kCustomField_None },
		{	95,		0x00000D00,	kCustomField_SubtractOne },
		{	100,	0x20800000,	kCustomField_None },
		{	101,	0x20800001,	kCustomField_None },
		{	102,	0x20800000,	kCustomField_None },
		{	103,	0x20800001,	kCustomField_None },
		{	110,	0x40800000,	kCustomField_None },
		{	111,	0x40800001,	kCustomField_None },
		{	112,	0x40800000,	kCustomField_None },
		{	113,	0x40800001,	kCustomField_None },
		{	120,	0x63000001,	kCustomField_None },
		{	121,	0x63000001,	kCustomField_DontApplyCPUField | kCustomField_SetBit },
		{	122,	0x63000001,	kCustomField_DontApplyCPUField | kCustomField_ClearBit },
		{	998,	0x00000006,	kCustomField_None },
		{	999,	0x60000000,	kCustomField_DontApplyCPUField },
		{	-1,		0x00000000,	kCustomField_End }
	};

	const struct ConversionTable * traverse = kConversionTable;

	UINT8	linkCheat = 0;
	UINT32	newCode;

	/* convert link cheats */
	if((code >= 500) && (code <= 699))
	{
		linkCheat = 1;
		code -= 500;
	}

	/* look up code */
	while(traverse->oldCode >= 0)
	{
		if(code == traverse->oldCode)
			goto foundCode;
		traverse++;
	}

	logerror("ConvertOldCode: %d not found\n", code);

	/* not found */
	*extendData = 0;
	return 0;

	/* found */
	foundCode:

	newCode = traverse->newCode;

	/* add in the CPU field */
	if(!(traverse->customField & kCustomField_DontApplyCPUField))
		newCode = (newCode & ~0x1F000000) | ((cpu << 24) & 0x1F000000);

	/* hack-ish, subtract one from data field for x5 user select */
	if(traverse->customField & kCustomField_SubtractOne)
		(*data)--;	// yaay for C operator precedence

	/* set up the extend data */
	if(traverse->customField & kCustomField_BitMask)
		*extendData = *data;
	else
		*extendData = 0xFFFFFFFF;

	if(traverse->customField & kCustomField_ClearBit)
		*data = 0;

	if(linkCheat)
		SET_MASK_FIELD(newCode, LinkEnable);

	return newCode;
}

/*-------------------
  ConvertToNewCode
-------------------*/

static int ConvertToNewCode(CheatAction * action)
{
	int newType = 0;

	if(EXTRACT_FIELD(action->type, LocationType) == kLocation_IndirectIndexed)
		SET_FIELD(newType, CPURegion, EXTRACT_FIELD(action->type, LocationParameterCPU));
	else if(EXTRACT_FIELD(action->type, LocationType) != kLocation_Custom)
		SET_FIELD(newType, CPURegion, EXTRACT_FIELD(action->type, LocationParameter));

	if(TEST_FIELD(action->type, UserSelectEnable))
		SET_FIELD(newType, CodeType, kCodeType_VWrite);
	else if(EXTRACT_FIELD(action->type, LocationType) == kLocation_IndirectIndexed)
	{	SET_FIELD(newType, CodeType, kCodeType_IWrite);
		SET_FIELD(newType, IndexSize, EXTRACT_FIELD(action->type, LocationParameterOption)); }
	else
	{	SET_FIELD(newType, CodeType, kCodeType_PDWWrite);
		SET_FIELD(newType, IndexSize, kSearchSize_32Bit); }

	SET_FIELD(newType, AddressSize, EXTRACT_FIELD(action->type, BytesUsed));

	if(TEST_FIELD(action->type, LinkEnable))
		SET_FIELD(newType, Link, kLink_Link);

	if(TEST_FIELD(action->type, RestorePreviousValue))
		SET_MASK_FIELD(newType, RestoreValue);

	if(EXTRACT_FIELD(action->type, Prefill))
		SET_MASK_FIELD(newType, PrefillEnable);

	if(!EXTRACT_FIELD(action->type, Type) && EXTRACT_FIELD(action->type, TypeParameter))
		SET_MASK_FIELD(newType, DelayEnable);

	if(TEST_FIELD(action->type, OneShot))
		SET_MASK_FIELD(newType, OneShot);

	return newType;
}

/*--------------------------
  HandleLocalCommandCheat
--------------------------*/

static void HandleLocalCommandCheat(running_machine *machine, int cpu, int type, int address, int data, UINT8 format)
{
	int command = 0;

	if(format != 1)
	{
		switch(EXTRACT_FIELD(type, LocationParameter))
		{
			case kCustomLocation_AssignActivationKey:
				command = kCustomCode_ActivationKey1;

				if(TEST_FIELD(type, OneShot))
					command = kCustomCode_ActivationKey2;
				break;

			case kCustomLocation_Enable:
				command = kCustomCode_PreEnable;
				break;

			case kCustomLocation_Overclock:
				command = kCustomCode_OverClock;
				break;

			case kCustomLocation_RefreshRate:
				command = kCustomCode_RefreshRate;
				break;

			default:
				return;
		}
	}
	else
		command = EXTRACT_FIELD(type, CodeType);

	switch(command)
	{
		case kCustomCode_ActivationKey1:
		case kCustomCode_ActivationKey2:
			if(address < cheatListLength)
			{
				CheatEntry * entry = &cheatList[address];

				if(command == kCustomCode_ActivationKey1)
				{
					entry->activationKey2 = data;
					entry->flags |= kCheatFlag_HasActivationKey2;
				}
				else
				{
					entry->activationKey1 = data;
					entry->flags |= kCheatFlag_HasActivationKey1;
				}
			}
			break;

		case kCustomCode_PreEnable:
			if(address < cheatListLength)
			{
				CheatEntry * entry = &cheatList[address];

				ActivateCheat(entry);

				if(data && (data < entry->actionListLength))
					entry->selection = data;
			}
			break;

		case kCustomCode_OverClock:
			if(VALID_CPU(cpu))
			{
				double	overclock = data / 65536.0;

				cpunum_set_clockscale(machine, address, overclock);
			}
			break;

		case kCustomCode_RefreshRate:
			{
				int width		= video_screen_get_width(machine->primary_screen);
				int height		= video_screen_get_height(machine->primary_screen);

				const rectangle	*visarea	= video_screen_get_visible_area(machine->primary_screen);
				double			refresh		= data / 65536.0;

				video_screen_configure(machine->primary_screen, width, height, visarea, HZ_TO_ATTOSECONDS(refresh));
			}
			break;
	}
}

/*----------------------------------------------------
  LoadCheatOption - load cheat option from database
----------------------------------------------------*/

static void LoadCheatOption(char * fileName)
{
	static char		buf[2048];
	mame_file *		theFile;
	file_error		filerr;
	cheat_format *	format = &cheat_format_table[0];

	/* open the database */
	filerr = mame_fopen(SEARCHPATH_CHEAT, fileName, OPEN_FLAG_READ, &theFile);

	if(filerr != FILERR_NONE)
	{
		messageType		= kCheatMessage_FailedToLoadDatabase;
		messageTimer	= DEFAULT_MESSAGE_TIME;
		return;
	}

	while(mame_fgets(buf, 2048, theFile))
	{
		char preCommand[9];

		if(sscanf(buf, format->format_string, preCommand) == format->arguments_matched)
		{
			if(strlen(preCommand) == format->length_matched)
			{
				sscanf(preCommand, "%X", &cheatOptions);
				messageType		= kCheatMessage_ReloadCheatOption;
				messageTimer	= DEFAULT_MESSAGE_TIME;
			}
		}
	}

	mame_fclose(theFile);
}

/*------------------------------------------------
  LoadCheatCode - load cheat code from database
------------------------------------------------*/

static void LoadCheatCode(running_machine *machine, char * fileName)
{
	enum{
		kFormatLevel_New = 1,
		kFormatLevel_Standard,
		kFormatLevel_Old };

	char		buf[2048];
	mame_file	* theFile;
	file_error	filerr;

	/* open the database */
	filerr = mame_fopen(SEARCHPATH_CHEAT, fileName, OPEN_FLAG_READ, &theFile);

	if(filerr != FILERR_NONE)
		return;

	foundCheatDatabase = 1;

	/* get a line from database */
	while(mame_fgets(buf, 2048, theFile))
	{
		int			i;
#ifdef MESS
		int			crc;
#endif
		int			type = 0;
		int			address = 0;
		int			data = 0;
		int			extendData = 0;
		int			argumentsMatched = 0;
		int			formatLevel = 0;
		int			oldCPU = 0;
		int			oldCode = 0;
		char		name[16] = { 0 };
		char		preData[16] = { 0 };
		char		preExtendData[16] = { 0 };
		char		description[256] = { 0 };
		char		comment[256] = { 0 };
		CheatEntry	* entry;
		CheatAction	* action;

		/* scan and check format */
		for(i = 1; i < 4; i++)
		{
			cheat_format * format = &cheat_format_table[i];

			/* scan a parameter */
			switch(i)
			{
				case kFormatLevel_New:
				case kFormatLevel_Standard:
#ifdef MESS
					argumentsMatched = sscanf(buf, format->format_string, name, &crc, &type, &address, preData, preExtendData, description, comment);
#else
					argumentsMatched = sscanf(buf, format->format_string, name, &type, &address, preData, preExtendData, description, comment);
#endif
					break;

				case kFormatLevel_Old:
#ifdef MESS
					argumentsMatched = sscanf(buf, format->format_string, &crc, name, &oldCPU, &address, &data, &oldCode, description, comment);
#else
					argumentsMatched = sscanf(buf, format->format_string, name, &oldCPU, &address, &data, &oldCode, description, comment);
#endif
			}

			/* NOTE : description and comment are not always needed */
			if(argumentsMatched < format->arguments_matched)
				continue;

#ifdef MESS
			/* check name */
			if(TEST_FIELD(cheatOptions, SharedCode) && strcmp(machine->gamedrv->parent, "0"))
				if(strcmp(name, machine->gamedrv->parent))
					break;
			else
#endif
			if(strcmp(name, machine->gamedrv->name))
				break;
#ifdef MESS
			/* check crc */
			if(MatchesCRCTable(crc) == 0)
				break;
#endif
			if(i != kFormatLevel_Old)
			{
				UINT8 hasError = 0;

				/* check the length of data and extendData */
				if(strlen(preData) != format->length_matched)
				{
					logerror("cheat - LoadCheatFile : %s has invalid extend data field length\n", description);
					messageType = kCheatMessage_WrongCode;
					hasError++;
				}

				if(strlen(preExtendData) != format->length_matched)
				{
					logerror("cheat - LoadCheatFile : %s has invalid data field length\n", description);
					messageType = kCheatMessage_WrongCode;
					hasError++;
				}

				if(hasError)
					continue;
				else
				{
					sscanf(preData, "%X", &data);
					sscanf(preExtendData, "%X", &extendData);
				}
			}
			else
				type = ConvertOldCode(oldCode, oldCPU, &data, &extendData);

			/* matched! */
			formatLevel = i;
			break;
		}

		/* attemp to get next line if unmatched */
		if(formatLevel == 0)
			continue;

		//logerror("cheat: processing %s\n", buf);

		/* handle command code */
		if(formatLevel == kFormatLevel_New)
		{
			if(EXTRACT_FIELD(type, CPURegion) == CUSTOM_CODE && EXTRACT_FIELD(type, CodeType) >= kCustomCode_ActivationKey1)
			{
				// logerror("cheat: cheat line removed\n", buf);
				HandleLocalCommandCheat(machine, EXTRACT_FIELD(type, CPUIndex), type, address, data, formatLevel);
				continue;
			}
		}
		else
		{
			if(	EXTRACT_FIELD(type, LocationType) == kLocation_Custom &&
				EXTRACT_FIELD(type, LocationParameter) <= kCustomLocation_AssignActivationKey)
				{
					HandleLocalCommandCheat(machine, EXTRACT_FIELD(type, LocationParameter), type, address, data, formatLevel);
					continue;
				}
		}

		/***** ENTRY *****/
		if(EXTRACT_FIELD(type, Link) == kLink_Master)
		{
			/* 1st (master) code in an entry */
			ResizeCheatList(cheatListLength + 1);

			if(cheatListLength == 0)
			{
				logerror("cheat - LoadCheatFile: cheat list resize failed; bailing\n");
				goto bail;
			}

			entry = &cheatList[cheatListLength - 1];

			entry->name = create_string_copy(description);

			if(argumentsMatched == cheat_format_table[formatLevel].comment_matched)
				entry->comment = create_string_copy(comment);
		}
		else
		{
			/* 2nd or later (link) code in an entry */
			if(cheatListLength == 0)
			{
				logerror("cheat - LoadCheatFile: first cheat found was link cheat; bailing\n");
				goto bail;
			}

			entry = &cheatList[cheatListLength - 1];
		}

		/***** ACTION *****/
		ResizeCheatActionList(&cheatList[cheatListLength - 1], entry->actionListLength + 1);

		if(entry->actionListLength == 0)
		{
			logerror("cheat - LoadCheatFile: action list resize failed; bailing\n");
			goto bail;
		}

		action = &entry->actionList[entry->actionListLength - 1];

		action->flags = 0;
		action->type = type;
		action->address = action->originalAddress = address;
		action->data = action->originalDataField = data;
		action->extendData = extendData;
		action->optionalName = create_string_copy(description);
		action->lastValue = NULL;

		if(formatLevel != kFormatLevel_New)
			action->flags |= kActionFlag_OldFormat;
	}

	bail:

	/* close the database */
	mame_fclose(theFile);
}

/*--------------------------------------------------------------------------------
  LoadUserDefinedSearchRegion - load user region code and rebuild search region
--------------------------------------------------------------------------------*/

static void LoadUserDefinedSearchRegion(running_machine *machine, char * fileName)
{
	int		count = 0;
	char	buf[2048];

	search_info 	*info = get_current_search();
	mame_file *		theFile;
	file_error		filerr;
	cheat_format *	format = &cheat_format_table[4];

	/* 1st, free all search regions in current search_info */
	dispose_search_reigons(info);

	/* 2nd, attempt to open the memorymap file */
	filerr = mame_fopen(SEARCHPATH_CHEAT, fileName, OPEN_FLAG_READ, &theFile);

	if(filerr != FILERR_NONE)
	{
		info->region_list_length	= 0;
		messageType				= kCheatMessage_FailedToLoadDatabase;
		messageTimer			= DEFAULT_MESSAGE_TIME;
		return;
	}

	/* 3rd, scan parameters from the file */
	while(mame_fgets(buf, 2048, theFile))
	{
#ifdef MESS
		int		crc;
#endif
		int		cpu;
		int		space;
		int		start;
		int		end;
		int		status;
		char	name[9];
		char	description[64];

		cpu_region_info *cpu_info;
		search_region	*region;

		description[0] = 0;

#ifdef MESS
		if(sscanf(buf, format->format_string, name, &crc, &cpu, &space, &start, &end, &status, description) < format->arguments_matched)
#else
		if(sscanf(buf, format->format_string, name, &cpu, &space, &start, &end, &status, description) < format->arguments_matched)
#endif
			continue;
		else
		{
#ifdef MESS
			if(!MatchesCRCTable(crc))
				continue;
#endif
			if(cpu != info->target_idx)
				continue;
		}

		cpu_info = get_cpu_info(cpu);

		/* check parameters */
		{
			switch(space)
			{
				case kAddressSpace_Program:			// standard program space
				case kAddressSpace_DirectMemory:	// direct memory access for standard program space
					/* check length */
					if(start >= end)
					{
						logerror("cheat - LoadUserDefinedSearchRegion : invalid length (start : %X >= end : %X) \n", start, end);
						continue;
					}

					/* check bound */
					if(end > cpu_info->address_mask)
					{
						logerror("cheat - LoadUserDefinedSearchRegion : end address (%X) is out of range\n", end);
						continue;
					}
					break;

				case kAddressSpace_DataSpace:	// data space
				case kAddressSpace_IOSpace:		// I/O space
					// underconstruction...
					break;

				default:
					logerror("cheat - LoadUserDefinedSearchRegion : %X is invalid space\n", space);
					continue;
			}

			/* check status */
			if(status > 1)
			{
				logerror("cheat - LoadUserDefinedSearchRegion : %X is invalid status\n", status);
				continue;
			}
		}

		/* allocate memory for new search region */
		info->region_list = realloc(info->region_list, (count + 1) * sizeof(search_region));

		if(!info->region_list)
		{
			/* memory allocation error */
			logerror("cheat - LoadUserDefinedSearchRegion: out of memory resizing search region\n");
			ui_popup_time(2, "out of memory while buiding user search region");
			info->region_list_length = 0;
			mame_fclose(theFile);
			return;
		}

		region = &info->region_list[count];

		/* 4th, set region parameters from scaned data */
		region->address		= start;
		region->length		= end - start + 1;
		region->target_idx	= cpu;

		if(!space)
		{
			region->target_type		= kRegionType_CPU;
			region->cached_pointer	= NULL;
		}
		else
		{
			region->target_type		= kRegionType_Memory;
			region->cached_pointer = GetMemoryRegionBasePointer(cpu, kAddressSpace_DirectMemory, start);

			//logerror("pointer for %s : %p\n", description, region->cachedPointer);
		}

		region->write_handler	= NULL;
		region->first			= NULL;
		region->last			= NULL;
		region->status			= NULL;
		region->backup_last		= NULL;
		region->backup_status	= NULL;
		region->flags			= status;

		sprintf(region->name,	"%.*X-%.*X %s",
								cpu_info->address_chars_needed, region->address,
								cpu_info->address_chars_needed, region->address + region->length - 1,
								description);

		count++;
	}

	info->region_list_length = count;

	if(info->region_list_length)
	{
		messageType		= kCheatMessage_ReloadUserRegion;
		messageTimer	= DEFAULT_MESSAGE_TIME;
	}

	mame_fclose(theFile);
}

/*---------------------------------------------------------
  LoadCheatDatabase - get the database name then load it
---------------------------------------------------------*/

static void LoadCheatDatabase(running_machine *machine, UINT8 flags)
{
	UINT8		first = 1;

	char		buf[256];
	char		data;
	const char	* inTraverse;
	char		* outTraverse;
	char		* mainTraverse;

	cheatfile = options_get_string(mame_options(), OPTION_CHEAT_FILE);

	if (!cheatfile[0])
		cheatfile = "cheat.dat";

	inTraverse		= cheatfile;
	outTraverse		= buf;
	mainTraverse	= mainDatabaseName;

	buf[0] = 0;

	do
	{
		data = *inTraverse;

		/* check separator or end */
		if(data == ';' || !data)
		{
			*outTraverse++ = 0;

			if(first)
				*mainTraverse++ = 0;

			if(buf[0])
			{
				/* load database based on the name we gotten */
				if(flags & kLoadFlag_CheatOption)
					LoadCheatOption(buf);
				if(flags & kLoadFlag_CheatCode)
					LoadCheatCode(machine, buf);
				if(flags & kLoadFlag_UserRegion)
					LoadUserDefinedSearchRegion(machine, buf);

				outTraverse	= buf;
				buf[0]		= 0;
				first		= 0;
				break;
			}
		}
		else
		{
			*outTraverse++ = data;

			if(first)
				*mainTraverse++ = data;
		}

		inTraverse++;
	}
	while(data);

	UpdateAllCheatInfo();
}

/*---------------------------------------------------------------------------------------
  DisposeCheatDatabase - deactivate all cheats when reload a code or exit cheat system
---------------------------------------------------------------------------------------*/

static void DisposeCheatDatabase(void)
{
	int i;

	/* first, turn all cheats "OFF" */
	for(i = 0; i < cheatListLength; i++)
		TempDeactivateCheat(&cheatList[i]);

	/* next, free memory for all cheat entries */
	if(cheatList)
	{
		for(i = 0; i < cheatListLength; i++)
			DisposeCheat(&cheatList[i]);

		free(cheatList);

		cheatList = NULL;
		cheatListLength = 0;
	}
}

/*-------------------------------------------------------------------------
  ReloadCheatDatabase - reload cheat database directly on the cheat menu
-------------------------------------------------------------------------*/

static void ReloadCheatDatabase(running_machine *machine)
{
	DisposeCheatDatabase();
	LoadCheatDatabase(machine, kLoadFlag_CheatCode);

	if(foundCheatDatabase)
		messageType	= kCheatMessage_ReloadCheatCode;
	else
		messageType	= kCheatMessage_FailedToLoadDatabase;

	messageTimer	= DEFAULT_MESSAGE_TIME;
}

/*------------------------------------------------------
  SaveCheatCode - save a cheat code into the database
------------------------------------------------------*/

static void SaveCheatCode(running_machine *machine, CheatEntry * entry)
{
#ifdef MESS
	INT8	i;
	INT8	error = 0;
	char	buf[4096];

	mame_file *	theFile;
	file_error	filerr;

	if(!entry || !entry->actionList || ((entry->flags & kCheatFlag_Select) && entry->actionListLength == 1))
		error = 1;

	/* open the database */
	if(!error)
	{
		filerr = mame_fopen(SEARCHPATH_CHEAT, mainDatabaseName, OPEN_FLAG_WRITE, &theFile);

		if(!theFile)
			error = 1;
		else if(filerr != FILERR_NONE)
		{
			filerr = mame_fopen(SEARCHPATH_CHEAT, mainDatabaseName, OPEN_FLAG_WRITE | OPEN_FLAG_CREATE | OPEN_FLAG_CREATE_PATHS, &theFile);

			if(filerr != FILERR_NONE)
				error = 1;
		}
	}

	if(error)
	{
		messageType = kCheatMessage_FailedToSave;
		messageTimer = DEFAULT_MESSAGE_TIME;
		return;
	}

	mame_fseek(theFile, 0, SEEK_END);

	/* write the code to the database */
	for(i = 0; i < entry->actionListLength; i++)
	{
		UINT8		addressLength	= 8;
		char		* name;
		char		* bufTraverse	= buf;

		CheatAction	* action		= &entry->actionList[i];

		/* get name */
		if(!i)
			name = entry->name;
		else
			name = action->optionalName;

		/* get address length of CPU/Region */
		addressLength = get_address_length(EXTRACT_FIELD(action->type, CPURegion));

		/* print format */
#ifdef MESS
		bufTraverse +=	sprintf(bufTraverse, ">%s:%.8X:%.8X:%.*X:%.8X:%.8X",
						machine->gamedrv->name,
						thisGameCRC,
						action->type,
						addressLength,
						action->originalAddress,
						action->originalDataField,
						action->extendData);
#else
		bufTraverse +=	sprintf(bufTraverse, ">%s:%.8X:%.*X:%.8X:%.8X",
						machine->gamedrv->name,
						action->type,
						addressLength,
						action->originalAddress,
						action->originalDataField,
						action->extendData);
#endif
		/* set name */
		if(name)
		{
			bufTraverse += sprintf(bufTraverse, ":%s", name);

			if(!i && entry->comment)
				bufTraverse += sprintf(bufTraverse, ":%s", entry->comment);
		}
		else
		{
			bufTraverse += sprintf(bufTraverse, ":(none)");

			if(!i && entry->comment)
				bufTraverse += sprintf(bufTraverse, ":%s", entry->comment);
		}

		/* set comment */
		if(!i && entry->comment)
			bufTraverse += sprintf(bufTraverse, ":%s", entry->comment);

		bufTraverse += sprintf(bufTraverse, "\n");

		mame_fwrite(theFile, buf, (UINT32)strlen(buf));
	}

	/* close the database */
	mame_fclose(theFile);

	/* clear dirty flag as "this entry is already saved" */
	entry->flags &= ~kCheatFlag_Dirty;

	messageType = kCheatMessage_SucceededToSave;
	messageTimer = DEFAULT_MESSAGE_TIME;
#endif
	SaveDescription(machine);
}

/*-----------------------------------------------------------------
  SaveActivationKey - save activation key code into the database
-----------------------------------------------------------------*/

static void SaveActivationKey(running_machine *machine, CheatEntry * entry, int entryIndex)
{
	INT8	i;
	INT8	error = 0;
	char	buf[4096];

	mame_file *	theFile = NULL;
	file_error	filerr;

	if(!(entry->flags & kCheatFlag_HasActivationKey1) && !(entry->flags & kCheatFlag_HasActivationKey2))
		error = 1;

	if(!error && (!entry || !entry->actionList || ((entry->flags & kCheatFlag_Select) && entry->actionListLength == 1)))
		error = 2;

	/* open the database */
	if(!error)
	{
		filerr = mame_fopen(SEARCHPATH_CHEAT, mainDatabaseName, OPEN_FLAG_WRITE, &theFile);

		if(!theFile)
			error = 2;
		else if(filerr != FILERR_NONE)
		{
			filerr = mame_fopen(SEARCHPATH_CHEAT, mainDatabaseName, OPEN_FLAG_WRITE | OPEN_FLAG_CREATE | OPEN_FLAG_CREATE_PATHS, &theFile);

			if(filerr != FILERR_NONE)
				error = 2;
		}
	}

	switch(error)
	{
		case 1:
			messageType = kCheatMessage_NoActivationKey;
			messageTimer = DEFAULT_MESSAGE_TIME;
			return;

		case 2:
			messageType = kCheatMessage_FailedToSave;
			messageTimer = DEFAULT_MESSAGE_TIME;
			return;
	}

	mame_fseek(theFile, 0, SEEK_END);

	/* write the code to the database */
	for(i = 0; i < 2; i++)
	{
		int		addressLength	= 8;
		int		activationKey	= !i ? entry->activationKey1 : entry->activationKey2;
		int		type			= 0;
		char	* bufTraverse	= buf;

		CheatAction	* action = &entry->actionList[0];
		astring		* name = input_code_name(astring_alloc(), activationKey);

		/* set the flag in type field */
		SET_FIELD(type, CPURegion, CUSTOM_CODE);

		if(activationKey)
			SET_FIELD(type, CodeType, kCustomCode_ActivationKey1);
		else
			SET_FIELD(type, CodeType, kCustomCode_ActivationKey2);

		/* get address length of CPU/Region */
		addressLength = get_address_length(EXTRACT_FIELD(action->type, CPURegion));

		/* if not find 1st activation key at saving 1st key, try to save 2nd key */
		if(!i && !(entry->flags & kCheatFlag_HasActivationKey1))
		{
			astring_free(name);
			continue;
		}

		/* if not find 2nd key after 1st key saved, finish */
		if(i && !(entry->flags & kCheatFlag_HasActivationKey2))
		{
			astring_free(name);
			break;
		}

#ifdef MESS
		bufTraverse +=	sprintf(bufTraverse,
						">%s:%.8X:%.8X:%.*X:%.8X:00000000",
						machine->gamedrv->name,
						thisGameCRC,
						type,
						addressLength,
						entryIndex,
						activationKey);
#else
		bufTraverse +=	sprintf(bufTraverse, ">%s:%.8X:%.*X:%.8X:00000000",
						machine->gamedrv->name,
						type,
						addressLength,
						entryIndex,
						activationKey);
#endif
		/* set description */
		if(!i)
		{
			if(!(entry->flags & kCheatFlag_Select))
				bufTraverse += sprintf(bufTraverse, ":Pre-selection Key ");
			else
				bufTraverse += sprintf(bufTraverse, ":1st Activation Key ");
		}
		else
		{
			if(!(entry->flags & kCheatFlag_Select))
				bufTraverse += sprintf(bufTraverse, ":Next-selection Key ");
			else
				bufTraverse += sprintf(bufTraverse, ":2nd Activation Key ");
		}

		/* set code name and button index */
		if(entry->name)
			bufTraverse += sprintf(bufTraverse, "for %s (%s)\n", entry->name, astring_c(name));
		else
			bufTraverse += sprintf(bufTraverse, "(%s)\n", astring_c(name));

		/* write the activation key code */
		mame_fwrite(theFile, buf, (UINT32)strlen(buf));

		astring_free(name);
	}

	/* close the database */
	mame_fclose(theFile);

	messageType = kCheatMessage_ActivationKeySaved;
	messageTimer = DEFAULT_MESSAGE_TIME;
}

/*-----------------------------------------------------------
  SavePreEnable - save a pre-enable code into the database
-----------------------------------------------------------*/

static void SavePreEnable(running_machine *machine, CheatEntry * entry, int entryIndex)
{
	int		type			= 0;
	INT8	error			= 0;
	INT8	addressLength	= 8;
	char	buf[4096];
	char	* bufTraverse	= buf;

	CheatAction		* action = &entry->actionList[0];
	mame_file *		theFile = NULL;
	file_error		filerr;

	if(!entry || !entry->actionList || ((entry->flags & kCheatFlag_Select) && entry->actionListLength == 1))
		error = 1;

	/* open the database */
	if(!error)
	{
		filerr = mame_fopen(SEARCHPATH_CHEAT, mainDatabaseName, OPEN_FLAG_WRITE, &theFile);

		if(!theFile)
			error = 1;
		else if(filerr != FILERR_NONE)
		{
			filerr = mame_fopen(SEARCHPATH_CHEAT, mainDatabaseName, OPEN_FLAG_WRITE | OPEN_FLAG_CREATE | OPEN_FLAG_CREATE_PATHS, &theFile);

			if(filerr != FILERR_NONE)
				error = 1;
		}
	}

	if(error)
	{
		messageType = kCheatMessage_FailedToSave;
		messageTimer = DEFAULT_MESSAGE_TIME;
		return;
	}
	else
	{
		SET_FIELD(type, CPURegion, CUSTOM_CODE);
		SET_FIELD(type, CodeType, kCustomCode_PreEnable);
	}

	mame_fseek(theFile, 0, SEEK_END);

	/* get address length of CPU/Region */
	addressLength = get_address_length(EXTRACT_FIELD(action->type, CPURegion));

#ifdef MESS
	bufTraverse +=	sprintf(bufTraverse, ">%s:%.8X:%8.8X:%.*X:%.8X:00000000",
					machine->gamedrv->name,
					thisGameCRC,
					type,
					addressLength,
					entryIndex,
					entry->selection);
#else
	bufTraverse +=	sprintf(bufTraverse, ">%s:%8.8X:%.*X:%.8X:00000000",
					machine->gamedrv->name,
					type,
					addressLength,
					entryIndex,
					entry->selection);
#endif

	/* set name */
	bufTraverse += sprintf(bufTraverse, ":Pre-enable");

	if(entry->name)
		bufTraverse += sprintf(bufTraverse, " for %s", entry->name);

	/* set label name */
	if(entry->selection)
		bufTraverse += sprintf(bufTraverse, " Label - %s", entry->actionList[entry->labelIndex[entry->selection]].optionalName);

	bufTraverse += sprintf(bufTraverse, "\n");

	/* write the pre-enable code */
	mame_fwrite(theFile, buf, (UINT32)strlen(buf));

	/* close the database */
	mame_fclose(theFile);

	messageType = kCheatMessage_PreEnableSaved;
	messageTimer = DEFAULT_MESSAGE_TIME;
}

/*--------------------------------------------------------------
  SaveCheatOptions - save cheat option code into the database
--------------------------------------------------------------*/

static void SaveCheatOptions(void)
{
	UINT8		error = 0;
	char		buf[32];

	mame_file *		theFile;
	file_error		filerr;

	/* open the database */
	filerr = mame_fopen(SEARCHPATH_CHEAT, mainDatabaseName, OPEN_FLAG_WRITE, &theFile);

	if(!theFile)
		error = 1;
	else if(filerr != FILERR_NONE)
	{
		filerr = mame_fopen(SEARCHPATH_CHEAT, mainDatabaseName, OPEN_FLAG_WRITE | OPEN_FLAG_CREATE | OPEN_FLAG_CREATE_PATHS, &theFile);

		if(filerr != FILERR_NONE)
			error = 1;
	}

	if(error)
	{
		messageType = kCheatMessage_FailedToSave;
		messageTimer = DEFAULT_MESSAGE_TIME;
		return;
	}

	mame_fseek(theFile, 0, SEEK_END);

	sprintf(buf, ":_command:%.8X\n", cheatOptions);

	/* write the command code */
	mame_fwrite(theFile, buf, (UINT32)strlen(buf));

	/* close the database */
	mame_fclose(theFile);

	messageType = kCheatMessage_SucceededToSave;
	messageTimer = DEFAULT_MESSAGE_TIME;
}

/*------------------
  SaveDescription
------------------*/

static void SaveDescription(running_machine *machine)
{
	UINT8	error = 0;
	char	buf[256];

	mame_file *		theFile;
	file_error		filerr;

	filerr = mame_fopen(SEARCHPATH_CHEAT, mainDatabaseName, OPEN_FLAG_WRITE, &theFile);

	if(!theFile)
		error = 1;
	else if(filerr != FILERR_NONE)
	{
		filerr = mame_fopen(SEARCHPATH_CHEAT, mainDatabaseName, OPEN_FLAG_WRITE | OPEN_FLAG_CREATE | OPEN_FLAG_CREATE_PATHS, &theFile);

		if(filerr != FILERR_NONE)
			error = 1;
	}

	if(error)
	{
		messageType = kCheatMessage_FailedToSave;
		messageTimer = DEFAULT_MESSAGE_TIME;
		return;
	}

	mame_fseek(theFile, 0, SEEK_END);
	sprintf(buf, "> [ %s ]\n", machine->gamedrv->description);

	/* write the command code */
	mame_fwrite(theFile, buf, (UINT32)strlen(buf));

	/* close the database */
	mame_fclose(theFile);

	messageType = kCheatMessage_SucceededToSave;
	messageTimer = DEFAULT_MESSAGE_TIME;
}

/*--------------------------------------------------------------------------
  DoAutoSaveCheat - save normal code automatically when exit cheat system
--------------------------------------------------------------------------*/

static void DoAutoSaveCheats(running_machine *machine)
{
	int i;

	for(i = 0; i < cheatListLength; i++)
	{
		CheatEntry * entry = &cheatList[i];

		/* if the entry is not edited/added newly or has not been already saved, save it */
		if(entry->flags & kCheatFlag_Dirty)
			SaveCheatCode(machine, entry);
	}
}

/*-------------------------------------------------------------------
  AddCheatFromResult - add a code from result viewer to cheat list
-------------------------------------------------------------------*/

static void AddCheatFromResult(search_info *search, search_region *region, UINT32 address)
{
	if((region->target_type == kRegionType_CPU) || (region->target_type == kRegionType_Memory))
	{
		CheatEntry	* entry = GetNewCheat();
		CheatAction	* action = &entry->actionList[0];
		char		tempString[1024];
		int			tempStringLength;
		UINT32		data = read_search_operand(kSearchOperand_First, search, region, address);

		tempStringLength = sprintf(tempString, "%.8X (%d) = %.*X", address, region->target_idx, kSearchByteDigitsTable[search->bytes], data);

		entry->name = realloc(entry->name, tempStringLength + 1);
		memcpy(entry->name, tempString, tempStringLength + 1);

		SET_FIELD(action->type, CPURegion, region->target_idx);
		SET_FIELD(action->type, AddressSize, kSearchByteIncrementTable[search->bytes] - 1);
		action->address = address;
		action->data = action->originalDataField = data;
		action->extendData = 0xFFFFFFFF;
		action->lastValue = NULL;

		UpdateCheatInfo(entry, 0);
	}
}

/*--------------------------------------------------------------------------------------------
  AddCheatFromFirstResult - add a code from search box to cheat list if found result is one
--------------------------------------------------------------------------------------------*/

static void AddCheatFromFirstResult(search_info *search)
{
	int	i;

	for(i = 0; i < search->region_list_length; i++)
	{
		search_region *region = &search->region_list[i];

		if(region->num_results)
		{
			UINT32	traverse;

			for(traverse = 0; traverse < region->length; traverse++)
			{
				UINT32	address = region->address + traverse;

				if(is_region_offset_valid(search, region, traverse))
				{
					AddCheatFromResult(search, region, address);
					return;
				}
			}
		}
	}
}

/*-------------------------------------------------------------------------
  AddWatchFromResult - add a watch code from result viewer to cheat list
-------------------------------------------------------------------------*/

static void AddWatchFromResult(search_info *search, search_region *region, UINT32 address)
{
	if(region->target_type == kRegionType_CPU || region->target_type == kRegionType_Memory)
	{
		WatchInfo * info = GetUnusedWatch();

		info->address			= address;
		info->cpu				= region->target_idx;
		info->numElements		= 1;
		info->elementBytes		= kWatchSizeConversionTable[search->bytes];
		info->labelType			= kWatchLabel_None;
		info->displayType		= kWatchDisplayType_Hex;
		info->skip				= 0;
		info->elementsPerLine	= 0;
		info->addValue			= 0;
		info->linkedCheat		= NULL;
		info->label[0]			= 0;

		messageType		= kCheatMessage_SucceededToAdd;
		messageTimer	= DEFAULT_MESSAGE_TIME;
	}
	else
	{
		messageType		= kCheatMessage_FailedToAdd;
		messageTimer	= DEFAULT_MESSAGE_TIME;
	}
}

/*---------------------
  search_sign_extend
---------------------*/

static UINT32 search_sign_extend(search_info *search, UINT32 value)
{
	if(search->sign)
		if(value & kSearchByteSignBitTable[search->bytes])
			value |= ~kSearchByteUnsignedMaskTable[search->bytes];

	return value;
}

/*---------------------------------------------------------------------
  read_search_operand - read a data from search region and return it
---------------------------------------------------------------------*/

static UINT32 read_search_operand(UINT8 type, search_info *search, search_region *region, UINT32 address)
{
	UINT32 value = 0;

	switch(type)
	{
		case kSearchOperand_Current:
			value = read_region_data(region, address - region->address, kSearchByteIncrementTable[search->bytes], search->swap);
			break;

		case kSearchOperand_Previous:
			value = DoMemoryRead(region->last, address - region->address, kSearchByteIncrementTable[search->bytes], search->swap, NULL);
			break;

		case kSearchOperand_First:
			value = DoMemoryRead(region->first, address - region->address, kSearchByteIncrementTable[search->bytes], search->swap, NULL);
			break;

		case kSearchOperand_Value:
			value = search->value;
	}

	value = search_sign_extend(search, value);

	return value;
}

/*---------------------------------------------------------------------------------------------
  read_search_operand_bit - read a bit data from search region and return it when bit search
---------------------------------------------------------------------------------------------*/

static UINT32 read_search_operand_bit(UINT8 type, search_info *search, search_region *region, UINT32 address)
{
	UINT32 value = 0;

	switch(type)
	{
		case kSearchOperand_Current:
			value = read_region_data(region, address - region->address, kSearchByteIncrementTable[search->bytes], search->swap);
			break;

		case kSearchOperand_Previous:
			value = DoMemoryRead(region->last, address - region->address, kSearchByteIncrementTable[search->bytes], search->swap, NULL);
			break;

		case kSearchOperand_First:
			value = DoMemoryRead(region->first, address - region->address, kSearchByteIncrementTable[search->bytes], search->swap, NULL);
			break;

		case kSearchOperand_Value:
			if(search->value)
				value = 0xFFFFFFFF;
			else
				value = 0x00000000;
	}

	value = search_sign_extend(search, value);

	return value;
}

/*------------------------------------------------------------
  do_search_comparison - compare data and return it matched
------------------------------------------------------------*/

static UINT8 do_search_comparison(search_info *search, UINT32 lhs, UINT32 rhs)
{
	INT32 svalue;

	if(search->sign)
	{
		/* signed */
		INT32	slhs = lhs;
		INT32	srhs = rhs;

		switch(search->comparison)
		{
			case kSearchComparison_LessThan:
				return slhs < srhs;

			case kSearchComparison_GreaterThan:
				return slhs > srhs;

			case kSearchComparison_EqualTo:
				return slhs == srhs;

			case kSearchComparison_LessThanOrEqualTo:
				return slhs <= srhs;

			case kSearchComparison_GreaterThanOrEqualTo:
				return slhs >= srhs;

			case kSearchComparison_NotEqual:
				return slhs != srhs;

			case kSearchComparison_IncreasedBy:
				svalue = search->value;

				if(search->value & kSearchByteSignBitTable[search->bytes])
					svalue |= ~kSearchByteUnsignedMaskTable[search->bytes];

				return slhs == (srhs + svalue);

			case kSearchComparison_NearTo:
				return (slhs == srhs) || ((slhs + 1) == srhs);
		}
	}
	else
	{
		/* unsigned */
		switch(search->comparison)
		{
			case kSearchComparison_LessThan:
				return lhs < rhs;

			case kSearchComparison_GreaterThan:
				return lhs > rhs;

			case kSearchComparison_EqualTo:
				return lhs == rhs;

			case kSearchComparison_LessThanOrEqualTo:
				return lhs <= rhs;

			case kSearchComparison_GreaterThanOrEqualTo:
				return lhs >= rhs;

			case kSearchComparison_NotEqual:
				return lhs != rhs;

			case kSearchComparison_IncreasedBy:
				svalue = search->value;

				if(search->value & kSearchByteSignBitTable[search->bytes])
					svalue |= ~kSearchByteUnsignedMaskTable[search->bytes];

				return lhs == (rhs + svalue);

			case kSearchComparison_NearTo:
				return (lhs == rhs) || ((lhs + 1) == rhs);
		}
	}

	return 0;
}

/*--------------------------------------------------------------------
  do_search_comparison_bit - compare bit data and return it matched
--------------------------------------------------------------------*/

static UINT32 do_search_comparison_bit(search_info *search, UINT32 lhs, UINT32 rhs)
{
	switch(search->comparison)
	{
		case kSearchComparison_LessThan:
		case kSearchComparison_NotEqual:
		case kSearchComparison_GreaterThan:
		case kSearchComparison_LessThanOrEqualTo:
		case kSearchComparison_GreaterThanOrEqualTo:
		case kSearchComparison_IncreasedBy:
			return lhs ^ rhs;

		case kSearchComparison_EqualTo:
		case kSearchComparison_NearTo:
			return ~(lhs ^ rhs);
	}

	return 0;
}

/*----------------------------------------------
  is_region_offset_valid - ????? unused ?????
----------------------------------------------*/

#if 0
static UINT8 is_region_offset_valid(search_info *search, search_region *region, UINT32 offset)
{
    switch(kSearchByteIncrementTable[search->bytes])
    {
        case 1:
            return *((UINT8  *)&region->status[offset]) == 0xFF;
            break;

        case 2:
            return *((UINT16 *)&region->status[offset]) == 0xFFFF;
            break;

        case 4:
            return *((UINT32 *)&region->status[offset]) == 0xFFFFFFFF;
            break;
    }

    return 0;
}
#endif

/*-----------------------------------------------------
  is_region_offset_valid_bit - check selected offset
-----------------------------------------------------*/

static UINT8 is_region_offset_valid_bit(search_info *search, search_region *region, UINT32 offset)
{
	switch(kSearchByteStep[search->bytes])
	{
		case 1:
			return *((UINT8  *)&region->status[offset]) != 0;
			break;

		case 2:
			return *((UINT16 *)&region->status[offset]) != 0;
			break;

		case 3:
			return ((*((UINT16 *)&region->status[offset]) != 0) | (*((UINT8 *)&region->status[offset+2]) != 0));
			break;

		case 4:
			return *((UINT32 *)&region->status[offset]) != 0;
			break;
	}

	return 0;
}

/*------------------------------------------------------------------
  invalidate_region_offset - remove unmatched offset after search
------------------------------------------------------------------*/

static void invalidate_region_offset(search_info *search, search_region *region, UINT32 offset)
{
	switch(kSearchByteStep[search->bytes])
	{
		case 1:
			*((UINT8  *)&region->status[offset]) = 0;
			break;

		case 2:
			*((UINT16 *)&region->status[offset]) = 0;
			break;

		case 3:
			*((UINT16 *)&region->status[offset]) = 0;
			*((UINT8  *)&region->status[offset+2]) = 0;
			break;

		case 4:
			*((UINT32 *)&region->status[offset]) = 0;
			break;
	}
}

/*--------------------------------------------------------------------------
  invalidate_region_offset_bit - remove unmatched offset after bit search
--------------------------------------------------------------------------*/

static void invalidate_region_offset_bit(search_info *search, search_region *region, UINT32 offset, UINT32 invalidate)
{
	switch(kSearchByteStep[search->bytes])
	{
		case 1:
			*((UINT8  *)&region->status[offset]) &= ~invalidate;
			break;

		case 2:
			*((UINT16 *)&region->status[offset]) &= ~invalidate;
			break;

		case 3:
			*((UINT16 *)&region->status[offset]) &= ~invalidate;
			*((UINT8  *)&region->status[offset+2]) &= ~invalidate;
			break;

		case 4:
			*((UINT32 *)&region->status[offset]) &= ~invalidate;
			break;
	}
}

/*----------------------------------------------------------------
  invalidate_entire_regions - invalidate selected search region
----------------------------------------------------------------*/

static void invalidate_entire_region(search_info *search, search_region *region)
{
	memset(region->status, 0, region->length);

	search->num_results -=	region->num_results;
	region->num_results =	0;
}

/*---------------------------------------------------------------
  init_new_search - initialize search region to start a search
---------------------------------------------------------------*/

static void init_new_search(search_info *search)
{
	int i;

	search->num_results = 0;

	for(i = 0; i < search->region_list_length; i++)
	{
		search_region *region = &search->region_list[i];

		if(region->flags & kRegionFlag_Enabled)
		{
			region->num_results = 0;

			memset(region->status, 0xFF, region->length);

			fill_buffer_from_region(region, region->first);

			memcpy(region->last, region->first, region->length);
		}
	}
}

/*----------------------------------------------------------------------------
  update_search - update a data in search region after initialize or search
----------------------------------------------------------------------------*/

static void update_search(search_info *search)
{
	int i;

	for(i = 0; i < search->region_list_length; i++)
	{
		search_region *region = &search->region_list[i];

		if(region->flags & kRegionFlag_Enabled)
			fill_buffer_from_region(region, region->last);
	}
}

/*---------------------------------------
  do_search - cheat engine search core
---------------------------------------*/

static void do_search(search_info *search)
{
	int i, j;

	search->num_results = 0;

	if(search->bytes == kSearchSize_1Bit)
	{
		/* bit search */
		for(i = 0; i < search->region_list_length; i++)
		{
			search_region	*region		= &search->region_list[i];
			UINT32			lastAddress	= region->length - kSearchByteIncrementTable[search->bytes] + 1;
			UINT32			increment	= kSearchByteStep[search->bytes];

			region->num_results = 0;

			if(region->length < kSearchByteIncrementTable[search->bytes] || (region->flags & kRegionFlag_Enabled) == 0)
				continue;

			for(j = 0; j < lastAddress; j += increment)
			{
				UINT32	address;
				UINT32	lhs, rhs;

				address = region->address + j;

				if(is_region_offset_valid_bit(search, region, j))
				{
					UINT32	validBits;

					lhs = read_search_operand_bit(search->lhs, search, region, address);
					rhs = read_search_operand_bit(search->rhs, search, region, address);

					/* do bit search */
					validBits = do_search_comparison_bit(search, lhs, rhs);

					invalidate_region_offset_bit(search, region, j, ~validBits);

					if(is_region_offset_valid_bit(search, region, j))
					{
						search->num_results++;
						region->num_results++;
					}
				}
			}
		}
	}
	else
	{
		/* normal search */
		for(i = 0; i < search->region_list_length; i++)
		{
			search_region	*region		= &search->region_list[i];
			UINT32			lastAddress	= region->length - kSearchByteIncrementTable[search->bytes] + 1;
			UINT32			increment	= kSearchByteStep[search->bytes];

			region->num_results = 0;

			if(region->length < kSearchByteIncrementTable[search->bytes] || (region->flags & kRegionFlag_Enabled) == 0)
				continue;

			for(j = 0; j < lastAddress; j += increment)
			{
				UINT32	address;
				UINT32	lhs, rhs;

				address = region->address + j;

				if(is_region_offset_valid(search, region, j))
				{
					lhs = read_search_operand(search->lhs, search, region, address);
					rhs = read_search_operand(search->rhs, search, region, address);

					/* do value search */
					if(do_search_comparison(search, lhs, rhs) == 0)
					{
						/* unmatched */
						invalidate_region_offset(search, region, j);
					}
					else
					{
						/* matched */
						search->num_results++;
						region->num_results++;
					}
				}
			}
		}
	}
}

/*-----------------------------------------------------------
  LookupHandlerMemory - search specified write handler
-----------------------------------------------------------*/

static UINT8 ** LookupHandlerMemory(UINT8 cpu, UINT32 address, UINT32 * outRelativeAddress)
{
	const address_map			* map = memory_get_address_map(cpu, ADDRESS_SPACE_PROGRAM);
	const address_map_entry		*entry;

	for(entry = map->entrylist; entry != NULL; entry = entry->next)
	{
		if(entry->write.generic != NULL && address >= entry->addrstart && address <= entry->addrend)
		{
			if(outRelativeAddress)
			{
				*outRelativeAddress = address - entry->addrstart;
				return (UINT8 **)entry->baseptr;
			}
		}
	}

	return NULL;
}

/*-------------------------------------------------------------------------------------------------------------
  GetMemoryRegionBasePointer - return base pointer to the base of RAM associated with given CPU and address
-------------------------------------------------------------------------------------------------------------*/

static UINT8 ** GetMemoryRegionBasePointer(UINT8 cpu, UINT8 space, UINT32 address)
{
	UINT8	* buf = NULL;

	switch(space)
	{
		case kAddressSpace_DataSpace:
			buf = memory_get_read_ptr(cpu, ADDRESS_SPACE_DATA, address);
			break;

		case kAddressSpace_IOSpace:
			buf = memory_get_read_ptr(cpu, ADDRESS_SPACE_IO, address);
			break;

		case kAddressSpace_OpcodeRAM:
			buf = memory_get_op_ptr(cpu, address, 0);
			break;

		case kAddressSpace_DirectMemory:
			buf = memory_get_read_ptr(cpu, ADDRESS_SPACE_PROGRAM, address);
			break;

		default:
			logerror("invalid address space (%x)\n", space);
	}

	if(buf)		buf -= address;

	//logerror("pointer (return) : %p\n", buf);

	return (UINT8 **)buf;
}

/*--------------------------------------------------------------------------------------------------------------
  DoCPURead - read a data from standard CPU region
              NOTE : if a driver has cpu_spinutil(), reading a data via cpunum_read_byte may conflict with it
--------------------------------------------------------------------------------------------------------------*/

static UINT32 DoCPURead(UINT8 cpu, UINT32 address, UINT8 bytes, UINT8 swap)
{
	cpu_region_info *info = get_cpu_info(cpu);

	address = DoShift(address, info->address_shift);

	switch(bytes)
	{
		case 1:
				return	(cpunum_read_byte(cpu, address + 0) <<  0);

		case 2:
			if(swap)
				return	(cpunum_read_byte(cpu, address + 0) <<  0) | (cpunum_read_byte(cpu, address + 1) <<  8);
			else
				return	(cpunum_read_byte(cpu, address + 0) <<  8) | (cpunum_read_byte(cpu, address + 1) <<  0);
			break;

		case 3:
			if(swap)
				return	(cpunum_read_byte(cpu, address + 0) <<  0) | (cpunum_read_byte(cpu, address + 1) <<  8) |
						(cpunum_read_byte(cpu, address + 2) << 16);
			else
				return	(cpunum_read_byte(cpu, address + 0) << 16) | (cpunum_read_byte(cpu, address + 1) <<  8) |
						(cpunum_read_byte(cpu, address + 2) <<  0);
			break;

		case 4:
			if(swap)
				return	(cpunum_read_byte(cpu, address + 0) <<  0) | (cpunum_read_byte(cpu, address + 1) <<  8) |
						(cpunum_read_byte(cpu, address + 2) << 16) | (cpunum_read_byte(cpu, address + 3) << 24);
			else
				return	(cpunum_read_byte(cpu, address + 0) << 24) | (cpunum_read_byte(cpu, address + 1) << 16) |
						(cpunum_read_byte(cpu, address + 2) <<  8) | (cpunum_read_byte(cpu, address + 3) <<  0);
			break;
	}

	return 0;
}

/*--------------------------------------------------
  DoMemoryRead - read a data from memory directly
--------------------------------------------------*/

static UINT32 DoMemoryRead(UINT8 * buf, UINT32 address, UINT8 bytes, UINT8 swap, cpu_region_info *info)
{
	int i;
	int data = 0;

	if(info)
		address = DoShift(address, info->address_shift);

	if(!info)
	{
		switch(bytes)
		{
			case 1:
				data = buf[address];
				break;

			case 2:
				data = *((UINT16 *)&buf[address]);

				if(swap)
					data =	((data >> 8) & 0x00FF) | ((data << 8) & 0xFF00);
				break;

			case 4:
				data = *((UINT32 *)&buf[address]);

				if(swap)
					data =	((data >> 24) & 0x000000FF) | ((data >>  8) & 0x0000FF00) |
							((data <<  8) & 0x00FF0000) | ((data << 24) & 0xFF000000);
				break;

			default:
				info = &raw_cpu_info;
				goto generic;
		}

		return data;
	}

generic:
	for(i = 0; i < bytes; i++)
		data |= swap ?	buf[SwapAddress(address + i, bytes, info)] << (i * 8) :
						buf[SwapAddress(address + i, bytes, info)] << ((bytes - i - 1) * 8);

	return data;
}

/*-----------------------------------------------------
  DoCPUWrite - write a data into standard CPU region
-----------------------------------------------------*/

static void DoCPUWrite(UINT32 data, UINT8 cpu, UINT32 address, UINT8 bytes, UINT8 swap)
{
	cpu_region_info *info = get_cpu_info(cpu);

	address = DoShift(address, info->address_shift);

	switch(bytes)
	{
		case 1:
				cpunum_write_byte(cpu, address + 0, data & 0xFF);
			break;

		case 2:
			if(swap)
			{
				cpunum_write_byte(cpu, address + 0, (data >> 0) & 0xFF);
				cpunum_write_byte(cpu, address + 1, (data >> 8) & 0xFF);
			}
			else
			{
				cpunum_write_byte(cpu, address + 0, (data >> 8) & 0xFF);
				cpunum_write_byte(cpu, address + 1, (data >> 0) & 0xFF);
			}
			break;

		case 3:
			if(swap)
			{
				cpunum_write_byte(cpu, address + 0, (data >>  0) & 0xFF);
				cpunum_write_byte(cpu, address + 1, (data >>  8) & 0xFF);
				cpunum_write_byte(cpu, address + 2, (data >> 16) & 0xFF);
			}
			else
			{
				cpunum_write_byte(cpu, address + 0, (data >> 16) & 0xFF);
				cpunum_write_byte(cpu, address + 1, (data >>  8) & 0xFF);
				cpunum_write_byte(cpu, address + 2, (data >>  0) & 0xFF);
			}
			break;

		case 4:
			if(swap)
			{
				cpunum_write_byte(cpu, address + 0, (data >>  0) & 0xFF);
				cpunum_write_byte(cpu, address + 1, (data >>  8) & 0xFF);
				cpunum_write_byte(cpu, address + 2, (data >> 16) & 0xFF);
				cpunum_write_byte(cpu, address + 3, (data >> 24) & 0xFF);
			}
			else
			{
				cpunum_write_byte(cpu, address + 0, (data >> 24) & 0xFF);
				cpunum_write_byte(cpu, address + 1, (data >> 16) & 0xFF);
				cpunum_write_byte(cpu, address + 2, (data >>  8) & 0xFF);
				cpunum_write_byte(cpu, address + 3, (data >>  0) & 0xFF);
			}
			break;

		default:
			logerror("DoCPUWrite: bad size (%d)\n", bytes);
			break;
	}
}

/*----------------------------------------------------
  DoMemoryWrite - write a data into memory directly
----------------------------------------------------*/

static void DoMemoryWrite(UINT32 data, UINT8 * buf, UINT32 address, UINT8 bytes, UINT8 swap, cpu_region_info *info)
{
	UINT32 i;

	if(info)
		address = DoShift(address, info->address_shift);

	if(!info)
	{
		switch(bytes)
		{
			case 1:
				buf[address] = data;
				break;

			case 2:
				if(swap)
					data =	((data >> 8) & 0x00FF) | ((data << 8) & 0xFF00);

				*((UINT16 *)&buf[address]) = data;
				break;

			case 4:
				if(swap)
					data =	((data >> 24) & 0x000000FF) | ((data >>  8) & 0x0000FF00) |
							((data <<  8) & 0x00FF0000) | ((data << 24) & 0xFF000000);

				*((UINT32 *)&buf[address]) = data;
				break;

			default:
				info = &raw_cpu_info;
				goto generic;
		}

		return;
	}

generic:
	for(i = 0; i < bytes; i++)
	{
		if(swap)	buf[SwapAddress(address + i, bytes, info)] = data >> (i * 8);
		else		buf[SwapAddress(address + i, bytes, info)] = data >> ((bytes - i - 1) * 8);
	}
}

/*-----------
  ReadData
-----------*/

static UINT32 ReadData(CheatAction * action)
{
	UINT8	addressRead		= EXTRACT_FIELD(action->type, AddressRead);
	UINT8	bytes			= EXTRACT_FIELD(action->type, AddressSize) + 1;

	if(addressRead != kReadFrom_Variable)
	{
		/* read a data from memory */
		int address = ~0;

		if(addressRead == kReadFrom_Address)		address = action->address;
		else if(addressRead == kReadFrom_Index)		address = cheatVariable[action->address];

		if(!TEST_FIELD(action->region, RegionFlag))
		{
			/* read a data from CPU */
			UINT8 cpu = EXTRACT_FIELD(action->region, CPUIndex);

			switch(EXTRACT_FIELD(action->region, AddressSpace))
			{
				case kAddressSpace_Program:
					if(action->flags & kActionFlag_IndexAddress)
					{
						int indexAddress;

						cpu_region_info *info = get_cpu_info(cpu);

						indexAddress = DoCPURead(cpu, address, EXTRACT_FIELD(action->type, IndexSize) + 1, cpu_needs_swap(cpu));

						if(info && info->address_shift)
							indexAddress = DoShift(indexAddress, info->address_shift);

						if(indexAddress)
						{
							indexAddress += action->extendData;
							return DoCPURead(cpu, indexAddress, bytes, cpu_needs_swap(cpu));
						}
					}
					else if(action->flags & kActionFlag_PDWWrite)
					{
						if(action->flags & kActionFlag_IsFirst)
						{
							address += bytes;
							bytes = EXTRACT_FIELD(action->type, IndexSize) + 1;
							action->flags &= ~kActionFlag_IsFirst;
						}
						else
							action->flags |= kActionFlag_IsFirst;

						return DoCPURead(cpu, address, bytes, cpu_needs_swap(cpu));
					}
					else
					{
						return DoCPURead(cpu, address, bytes, cpu_needs_swap(cpu));
					}
					break;

				case kAddressSpace_DataSpace:
				case kAddressSpace_IOSpace:
				case kAddressSpace_OpcodeRAM:
				case kAddressSpace_DirectMemory:
					{
						UINT8 * buf;

						action->cachedPointer = GetMemoryRegionBasePointer(cpu, EXTRACT_FIELD(action->type, AddressSpace), address);
						buf = (UINT8 *) action->cachedPointer;

						if(buf)
						{
							if(action->flags && kActionFlag_PDWWrite)
							{
								if(action->flags & kActionFlag_IsFirst)
								{
									address += bytes;
									bytes = EXTRACT_FIELD(action->type, IndexSize) + 1;
									action->flags &= ~kActionFlag_IsFirst;
								}
								else
									action->flags |= kActionFlag_IsFirst;
							}

							return DoMemoryRead(buf, address, bytes, cpu_needs_swap(cpu), get_cpu_info(cpu));
						}
					}
					break;

				case kAddressSpace_MappedMemory:
					{
						int			relativeAddress;
						UINT8 **	buf;

						action->cachedPointer = LookupHandlerMemory(cpu, address, &action->cachedOffset);
						buf = action->cachedPointer;
						relativeAddress = action->cachedOffset;

						if(buf && *buf)
						{
							if(action->flags & kActionFlag_PDWWrite)
							{
								if(action->flags & kActionFlag_IsFirst)
								{
									relativeAddress += bytes;
									bytes = EXTRACT_FIELD(action->type, IndexSize) + 1;
									action->flags &= ~kActionFlag_IsFirst;
								}
								else
									action->flags |= kActionFlag_IsFirst;
							}

							return DoMemoryRead(*buf, relativeAddress, bytes, cpu_needs_swap(cpu), get_cpu_info(cpu));
						}
					}
					break;

				case kAddressSpace_EEPROM:
					{
						int		length;
						UINT8 *	buf;

						buf = eeprom_get_data_pointer(&length);

						if(IsAddressInRange(action, length))
						{
							if(action->flags & kActionFlag_PDWWrite)
							{
								if(action->flags & kActionFlag_IsFirst)
								{
									address += bytes;
									bytes = EXTRACT_FIELD(action->type, IndexSize) + 1;
									action->flags &= ~kActionFlag_IsFirst;
								}
								else
									action->flags |= kActionFlag_IsFirst;
							}

							return DoMemoryRead(buf, address, bytes, 0, &raw_cpu_info);
						}
					}
					break;
			}
		}
		else
		{
			/* read a data from region */
			UINT8 * buf = memory_region(action->region);

			if(buf && IsAddressInRange(action, memory_region_length(action->region)))
			{
				if(action->flags & kActionFlag_PDWWrite)
				{
					if(action->flags & kActionFlag_IsFirst)
					{
						address += bytes;
						bytes = EXTRACT_FIELD(action->type, IndexSize) + 1;
						action->flags &= ~kActionFlag_IsFirst;
					}
					else
						action->flags |= kActionFlag_IsFirst;

					return DoMemoryRead(buf, address, bytes, region_needs_swap(action->region), get_region_info(action->region));
				}
				else
					return DoMemoryRead(buf, address, bytes, region_needs_swap(action->region), get_region_info(action->region));
			}
		}
	}
	else
		/* read a data from variable */
		return (cheatVariable[action->address] & kCheatSizeMaskTable[EXTRACT_FIELD(action->type, AddressSize)]);

	return 0;
}

/*-------------------------------------
  WriteData - write a data to memory
-------------------------------------*/

static void WriteData(CheatAction * action, UINT32 data)
{
	UINT8	addressRead =	EXTRACT_FIELD(action->type, AddressRead);
	UINT8	bytes =			EXTRACT_FIELD(action->type, AddressSize) + 1;

	if(EXTRACT_FIELD(action->type, AddressRead) != kReadFrom_Variable)
	{
		/* write a data to memory */
		int address = ~0;

		if(addressRead == kReadFrom_Address)		address = action->address;
		else if(addressRead == kReadFrom_Index)		address = cheatVariable[action->address];

		if(!TEST_FIELD(action->region, RegionFlag))
		{
			UINT8 cpu = EXTRACT_FIELD(action->region, CPUIndex);

			switch(EXTRACT_FIELD(action->region, AddressSpace))
			{
				case kAddressSpace_Program:
					if(action->flags & kActionFlag_IndexAddress)
					{
						int indexAddress;

						cpu_region_info *info = get_cpu_info(cpu);

						indexAddress = DoCPURead(cpu, address, EXTRACT_FIELD(action->type, IndexSize) + 1, cpu_needs_swap(cpu));

						if(info && info->address_shift)
							indexAddress = DoShift(indexAddress, info->address_shift);

						if(indexAddress)
						{
							indexAddress += action->extendData;
							DoCPUWrite(data, cpu, indexAddress, bytes, cpu_needs_swap(cpu));
						}
					}
					else if(action->flags & kActionFlag_PDWWrite)
					{
						if(action->flags & kActionFlag_IsFirst)
						{
							address += bytes;
							bytes = EXTRACT_FIELD(action->type, IndexSize) + 1;
							action->flags &= ~kActionFlag_IsFirst;
						}
						else
							action->flags |= kActionFlag_IsFirst;

						DoCPUWrite(data, cpu, address, bytes, cpu_needs_swap(cpu));
					}
					else
					{
						DoCPUWrite(data, cpu, address, bytes, cpu_needs_swap(cpu));
					}
					break;

				case kAddressSpace_DataSpace:
				case kAddressSpace_IOSpace:
				case kAddressSpace_OpcodeRAM:
				case kAddressSpace_DirectMemory:
					{
						UINT8 * buf;

						action->cachedPointer = GetMemoryRegionBasePointer(cpu, EXTRACT_FIELD(action->type, AddressSpace), address);
						buf = (UINT8 *) action->cachedPointer;

						if(buf)
						{
							if(action->flags & kActionFlag_PDWWrite)
							{
								if(action->flags & kActionFlag_IsFirst)
								{
									address += bytes;
									bytes = EXTRACT_FIELD(action->type, IndexSize) + 1;
									action->flags &= ~kActionFlag_IsFirst;
								}
								else
									action->flags |= kActionFlag_IsFirst;
							}

							DoMemoryWrite(data, buf, address, bytes, cpu_needs_swap(cpu), get_cpu_info(cpu));
						}
					}
					break;

				case kAddressSpace_MappedMemory:
					{
						int			relativeAddress;
						UINT8 **	buf;

						action->cachedPointer = LookupHandlerMemory(cpu, address, &action->cachedOffset);
						buf = action->cachedPointer;
						relativeAddress = action->cachedOffset;

						if(buf && *buf)
						{
							if(action->flags & kActionFlag_PDWWrite)
							{
								if(action->flags & kActionFlag_IsFirst)
								{
									relativeAddress += bytes;
									bytes = EXTRACT_FIELD(action->type, IndexSize) + 1;
									action->flags &= ~kActionFlag_IsFirst;
								}
								else
									action->flags |= kActionFlag_IsFirst;
							}

							DoMemoryWrite(data, *buf, relativeAddress, bytes, cpu_needs_swap(cpu), get_cpu_info(cpu));
							return;
						}
					}
					break;

				case kAddressSpace_EEPROM:
					{
						int		length;
						UINT8 *	buf;

						buf = eeprom_get_data_pointer(&length);

						if(IsAddressInRange(action, length))
						{
							if(action->flags & kActionFlag_PDWWrite)
							{
								if(action->flags & kActionFlag_IsFirst)
								{
									address += bytes;
									bytes = EXTRACT_FIELD(action->type, IndexSize) + 1;
									action->flags &= ~kActionFlag_IsFirst;
								}
								else
									action->flags |= kActionFlag_IsFirst;
							}

							DoMemoryWrite(data, buf, address, bytes, 0, &raw_cpu_info);
							return;
						}
					}
					break;
			}
		}
		else
		{
			/* write a data into region */
			UINT8 * buf = memory_region(action->region);

			if(buf && IsAddressInRange(action, memory_region_length(action->region)))
			{
				if(action->flags & kActionFlag_PDWWrite)
				{
					if(action->flags & kActionFlag_IsFirst)
					{
						address += bytes;
						bytes = EXTRACT_FIELD(action->type, IndexSize) + 1;
						action->flags &= ~kActionFlag_IsFirst;
					}
					else
						action->flags |= kActionFlag_IsFirst;
				}

				DoMemoryWrite(data, buf, address, bytes, region_needs_swap(action->region), get_region_info(action->region));
				return;
			}
		}
	}
	else
		cheatVariable[action->address] = data & kCheatSizeMaskTable[EXTRACT_FIELD(action->type, AddressSize)];
}

/*------------------------------------------------------------------------------------
  WatchCheatEntry - set watchpoint when watch value key is pressed in several menus
------------------------------------------------------------------------------------*/

static void WatchCheatEntry(CheatEntry * entry, UINT8 associate)
{
	int			i, j;
	CheatEntry	* associateEntry = NULL;

	/* NOTE : calling with associate = 1 doesn't exist right now */
	if(associate)
		associateEntry = entry;

	if(!entry)
	{
		messageType = kCheatMessage_FailedToAdd;
		messageTimer = DEFAULT_MESSAGE_TIME;
		return;
	}

	for(i = 0; i < entry->actionListLength; i++)
	{
		CheatAction * action = &entry->actionList[i];

		if(!(action->flags & kActionFlag_NoAction) && !EXTRACT_FIELD(action->type, AddressRead))
		{
			/* NOTE : unsupported the watchpoint for indirect index right now */
			if((action->flags & kActionFlag_IndexAddress) && action->extendData != ~0)
				continue;

			if(!i)
			{
				/* first CheatAction */
				AddActionWatch(&entry->actionList[i], associateEntry);
			}
			else
			{
				/* 2nd or later CheatAction */
				UINT8 differentAddress = 1;

				for(j = 0; j < i; j++)
				{
					/* if we find the same address CheatAction, skip it */
					if(entry->actionList[j].address == entry->actionList[i].address)
						differentAddress = 0;
				}

				if(differentAddress)
					AddActionWatch(&entry->actionList[i], associateEntry);
			}
		}
	}

	messageType = kCheatMessage_SucceededToAdd;
	messageTimer = DEFAULT_MESSAGE_TIME;
}

/*-----------------------------------------------------------------------
  AddActionWatch - set watchpoint parameters into WatchInfo.
                   called from WatchCheatEntry() when watch value key
                   or ActivateCheat() via watchpoint code.
-----------------------------------------------------------------------*/

static void AddActionWatch(CheatAction * action, CheatEntry * entry)
{
	switch(EXTRACT_FIELD(action->type, CodeType))
	{
		case kCodeType_Write:
		case kCodeType_PDWWrite:
		case kCodeType_RWrite:
		case kCodeType_IWrite:
		case kCodeType_CWrite:
		case kCodeType_VWrite:
		case kCodeType_VRWrite:
		case kCodeType_Branch:
		case kCodeType_Move:
		case kCodeType_Loop:
		case kCodeType_Popup:
			{
				WatchInfo	* info = GetUnusedWatch();

				info->address		= action->address;
				info->cpu			= action->region;
				info->displayType	= kWatchDisplayType_Hex;
				info->elementBytes	= kSearchSize_8Bit;
				info->label[0]		= 0;
				info->labelType		= kWatchLabel_None;
				info->numElements	= EXTRACT_FIELD(action->type, AddressSize) + 1;
				info->linkedCheat	= entry;
				info->skip			= 0;

				if(action->flags & kActionFlag_PDWWrite)
				{
					info->numElements += (EXTRACT_FIELD(action->type, IndexSize) + 1);
				}
				else if(action->flags & kActionFlag_Repeat)
				{
					info->numElements	= EXTRACT_FIELD(action->extendData, LSB16);
					info->skip			= EXTRACT_FIELD(action->extendData, MSB16) - 1;
				}
			}
			break;

		case kCodeType_Watch:
			{
				WatchInfo * info = GetUnusedWatch();

				info->address			= action->address;
				info->cpu				= EXTRACT_FIELD(action->type, CPURegion);
				info->displayType		= EXTRACT_FIELD(action->type, WatchDisplayFormat);
				info->elementBytes		= kByteConversionTable[EXTRACT_FIELD(action->type, AddressSize)];
				info->label[0]			= 0;
				info->linkedCheat		= entry;
				info->numElements		= EXTRACT_FIELD(action->data, WatchNumElements) + 1;
				info->skip				= EXTRACT_FIELD(action->data, WatchSkip);
				info->elementsPerLine	= EXTRACT_FIELD(action->data, WatchElementsPerLine);
				info->addValue			= EXTRACT_FIELD(action->data, WatchAddValue);

				if(info->addValue & 0x80)
					info->addValue |= ~0xFF;

				if(action->extendData != ~0)
				{
					/* ### fix me... ### */
					info->x = (float)(EXTRACT_FIELD(action->extendData, LSB16)) / 100;
					info->y = (float)(EXTRACT_FIELD(action->extendData, MSB16)) / 100;
				}

				if(TEST_FIELD(action->type, WatchShowLabel) && entry->comment && strlen(entry->comment) < 256)
				{
					info->labelType = kWatchLabel_String;
					strcpy(info->label, entry->comment);
				}
				else
					info->labelType = kWatchLabel_None;
			}
			break;
	}
}

/*------------------------------------------------------------------------------
  RemoveAssociatedWatches - delete watchpoints when watchpoint code is "OFF".
                            called from DeactivateCheat().
------------------------------------------------------------------------------*/

static void RemoveAssociatedWatches(CheatEntry * entry)
{
	int i;

	for(i = watchListLength - 1; i >= 0; i--)
	{
		WatchInfo * info = &watchList[i];

		if(info->linkedCheat == entry)
			DeleteWatchAt(i);
	}
}

/*--------------------------------------------------
  ResetAction - back up data and set action flags
--------------------------------------------------*/

static void ResetAction(CheatAction * action)
{
	if(action->flags & kActionFlag_NoAction)
		return;

	/* back up a value */
	if(!(action->flags & kActionFlag_LastValueGood))
	{
		switch(EXTRACT_FIELD(action->type, CodeType))
		{
			default:
				action->lastValue = malloc_or_die(sizeof(action->lastValue));
				action->lastValue[0] = ReadData(action);
				break;

			case kCodeType_PDWWrite:
				action->lastValue = malloc_or_die(sizeof(action->lastValue) * 2);
				action->lastValue[0] = ReadData(action);
				action->lastValue[1] = ReadData(action);
				break;

			case kCodeType_RWrite:
			case kCodeType_VRWrite:
				{
					int i;

					for(i = 0; i < EXTRACT_FIELD(action->extendData, LSB16); i++)
					{
						action->lastValue = realloc(action->lastValue, sizeof(action->lastValue) * EXTRACT_FIELD(action->extendData, LSB16));
						action->lastValue[i] = ReadData(action);
						action->address +=	EXTRACT_FIELD(action->extendData, MSB16) ?
											EXTRACT_FIELD(action->extendData, MSB16) :
											kSearchByteIncrementTable[EXTRACT_FIELD(action->type, AddressSize)];
					}
					action->address = action->originalAddress;
				}
				break;
		}
	}

	action->frameTimer = 0;
	action->flags &= ~kActionFlag_StateMask;
	action->flags |= kActionFlag_LastValueGood;

	if(action->flags & kActionFlag_CheckCondition)
	{
		if(EXTRACT_FIELD(action->type, CodeParameter) == kCondition_PreviousValue)
		{
			if(EXTRACT_FIELD(action->type, CodeType) == kCodeType_CWrite)
				action->extendData = ReadData(action);
			else
				action->data = ReadData(action);
		}
	}
}

/*-------------------------------------------------------------------------------------
  ActivateCheat - reset action entry and set activate entry flag when turn CODE "ON"
-------------------------------------------------------------------------------------*/

static void ActivateCheat(CheatEntry * entry)
{
	int	i;

	for(i = 0; i < entry->actionListLength; i++)
	{
		CheatAction * action = &entry->actionList[i];

		ResetAction(action);

		/* if watchpoint code, add watchpoint */
		if(EXTRACT_FIELD(action->type, CodeType) == kCodeType_Watch)
			AddActionWatch(action, entry);
	}

	/* set activate flag */
	entry->flags |= kCheatFlag_Active;
}

/*--------------------------------------------------------------------------------------
  DeactivateCheat - restore previous value and remove watchpoint when turn CODE "OFF"
--------------------------------------------------------------------------------------*/

static void DeactivateCheat(CheatEntry * entry)
{
	int i;

	for(i = 0; i < entry->actionListLength; i++)
	{
		CheatAction * action = &entry->actionList[i];

		/* restore previous value and clear backup flag if needed */
		if(TEST_FIELD(action->type, RestoreValue) && (action->flags & kActionFlag_LastValueGood))
		{
			switch(EXTRACT_FIELD(action->type, CodeType))
			{
				default:
					WriteData(action, (UINT32)action->lastValue[0]);
					break;

				case kCodeType_PDWWrite:
					WriteData(action, (UINT32)action->lastValue[0]);
					WriteData(action, (UINT32)action->lastValue[1]);
					break;

				case kCodeType_RWrite:
				case kCodeType_VRWrite:
					{
						int j;

						for(j = 0; j < EXTRACT_FIELD(action->extendData, LSB16); j++)
						{
							WriteData(action, (UINT32)action->lastValue[j]);
							action->address +=	EXTRACT_FIELD(action->extendData, MSB16) ?
												EXTRACT_FIELD(action->extendData, MSB16) :
												kSearchByteIncrementTable[EXTRACT_FIELD(action->type, AddressSize)];
						}
						action->address = action->originalAddress;
					}
					break;
			}
			action->flags &= ~kActionFlag_LastValueGood;
		}
		free(action->lastValue);
		action->lastValue = NULL;
	}

	/* remove watchpoint */
	RemoveAssociatedWatches(entry);

	/* clear active flag */
	entry->flags &= ~kCheatFlag_StateMask;
}

/*--------------------------------------------------------------------
  TempDeactivateCheat - restore previous value when turn CHEAT "OFF"
--------------------------------------------------------------------*/

static void TempDeactivateCheat(CheatEntry * entry)
{
	if(entry->flags & kCheatFlag_Active)
	{
		int i;

		for(i = 0; i < entry->actionListLength; i++)
		{
			CheatAction * action = &entry->actionList[i];

			action->frameTimer = 0;
			action->flags &= ~kActionFlag_OperationDone;

			/* restore previous value if needed */
			switch(EXTRACT_FIELD(action->type, CodeType))
			{
				default:
					WriteData(action, (UINT32)action->lastValue[0]);
					break;

				case kCodeType_PDWWrite:
					WriteData(action, (UINT32)action->lastValue[0]);
					WriteData(action, (UINT32)action->lastValue[1]);
					break;

				case kCodeType_RWrite:
				case kCodeType_VRWrite:
					{
						int j;

						for(j = 0; j < EXTRACT_FIELD(action->extendData, LSB16); j++)
						{
							WriteData(action, (UINT32)action->lastValue[j]);
							action->address +=	EXTRACT_FIELD(action->extendData, MSB16) ?
												EXTRACT_FIELD(action->extendData, MSB16) :
												kSearchByteIncrementTable[EXTRACT_FIELD(action->type, AddressSize)];
						}
						action->address = action->originalAddress;
					}
					break;
			}
		}
	}
}


/*------------------------------------------------------------
  cheat_periodicOperation - management for cheat operations
------------------------------------------------------------*/

static void cheat_periodicOperation(CheatAction * action)
{
	int data = TEST_FIELD(action->type, DataRead) ? cheatVariable[action->data] : action->data;

	if(action->flags & kActionFlag_PDWWrite)
	{
		action->flags &= ~kActionFlag_IsFirst;
		WriteData(action, data);
		WriteData(action, action->extendData);
	}
	else if(action->flags & kActionFlag_Repeat)
	{
		int i;

		for(i = 0; i < EXTRACT_FIELD(action->extendData, LSB16); i++)
		{
			WriteData(action, data);
			action->address +=	EXTRACT_FIELD(action->extendData, MSB16) ?
								EXTRACT_FIELD(action->extendData, MSB16) :
								kSearchByteIncrementTable[EXTRACT_FIELD(action->type, AddressSize)];
		}
		action->address = action->originalAddress;
	}
	else
	{
		switch(EXTRACT_FIELD(action->type, CodeType))
		{
			case kCodeType_Write:
				WriteData(action, (data & action->extendData) | (ReadData(action) & ~action->extendData));
				break;

			case kCodeType_IWrite:
				switch(EXTRACT_FIELD(action->type, CodeParameter))
				{
					case kIWrite_Write:
						WriteData(action, data);
						break;

					case kIWrite_BitSet:
						WriteData(action, ReadData(action) | data);
						break;

					case kIWrite_BitClear:
						WriteData(action, ReadData(action) & ~data);
						break;

					case kIWrite_LimitedMask:
						WriteData(action, (EXTRACT_FIELD(data, MSB16) & EXTRACT_FIELD(data, LSB16)) | (ReadData(action) & ~EXTRACT_FIELD(data, LSB16)));
						break;
				}
				break;

			case kCodeType_CWrite:
			case kCodeType_VWrite:
				WriteData(action, data);
				break;

			case kCodeType_Move:
				cheatVariable[EXTRACT_FIELD(action->type, CodeParameter)] = ReadData(action) + data;
				break;

			case kCodeType_Popup:
				switch(EXTRACT_FIELD(action->type, PopupParameter))
				{
					case kPopup_Label:
						ui_popup_time(1, "%s", action->optionalName);
						break;

					case kPopup_Value:
						ui_popup_time(1, "%*.*X",
										kCheatSizeDigitsTable[EXTRACT_FIELD(action->type, AddressRead)],
										kCheatSizeDigitsTable[EXTRACT_FIELD(action->type, AddressRead)],
										ReadData(action));
						break;

					case kPopup_LabelValue:
						ui_popup_time(1, "%s %*.*X",
										action->optionalName,
										kCheatSizeDigitsTable[EXTRACT_FIELD(action->type, AddressRead)],
										kCheatSizeDigitsTable[EXTRACT_FIELD(action->type, AddressRead)],
										ReadData(action));
						break;

					case kPopup_ValueLabel:
						ui_popup_time(1, "%*.*X %s",
										kCheatSizeDigitsTable[EXTRACT_FIELD(action->type, AddressRead)],
										kCheatSizeDigitsTable[EXTRACT_FIELD(action->type, AddressRead)],
										ReadData(action),
										action->optionalName);
						break;
				}
				break;
#ifdef MAME_DEBUG
			default:
				ui_popup_time(1, "Invalid CodeType : %X", EXTRACT_FIELD(action->type, CodeType));
#endif
		}
	}
}

/*------------------------------------------------------------
  cheat_periodicCondition - management for cheat conditions
                            used by CWrite, Branch, Popup
------------------------------------------------------------*/

static UINT8 cheat_periodicCondition(CheatAction * action)
{
	int	data	= ReadData(action);
	int	value	= EXTRACT_FIELD(action->type, CodeType) == kCodeType_CWrite ? action->extendData : action->data;

	switch(EXTRACT_FIELD(action->type, CodeParameter))
	{
		case kCondition_Equal:
			return (data == value);

		case kCondition_NotEqual:
			return (data != value);

		case kCondition_Less:
			return (data < value);

		case kCondition_LessOrEqual:
			return (data <= value);

		case kCondition_Greater:
			return (data > value);

		case kCondition_GreaterOrEqual:
			return (data >= value);

		case kCondition_BitSet:
			return (data & value);

		case kCondition_BitClear:
			return (!(data & value));

		case kCondition_PreviousValue:
			if(data != value)
			{
				if(EXTRACT_FIELD(action->type, CodeType) == kCodeType_CWrite)
					action->extendData = data;
				else
					action->data = data;

				return 1;
			}
			break;

		case kCondition_KeyPressedOnce:
			return (input_code_pressed_once(value));

		case kCondition_KeyPressedRepeat:
			return (input_code_pressed(value));

		case kCondition_True:
			return 1;
	}

	return 0;
}

/*------------------------------------------------------
  cheat_periodicAction - management for cheat actions
------------------------------------------------------*/

static int cheat_periodicAction(running_machine *machine, CheatAction * action, int selection)
{
	UINT8 executeOperation = 0;

	if(driverSpecifiedFlag)
		return (TEST_FIELD(action->type, Return) ? CHEAT_RETURN_VALUE : selection + 1);

	if(TEST_FIELD(action->type, PrefillEnable) && !(action->flags & kActionFlag_PrefillDone))
	{
		UINT8 prefillValue = kPrefillValueTable[EXTRACT_FIELD(action->type, PrefillEnable)];

		if(!(action->flags & kActionFlag_PrefillWritten))
		{
			/* set prefill */
			WriteData(action, prefillValue);
			action->flags |= kActionFlag_PrefillWritten;
			return (TEST_FIELD(action->type, Return) ? CHEAT_RETURN_VALUE : selection + 1);

		}
		else
		{
			/* do re-write */
			if(ReadData(action) == prefillValue)
				return (TEST_FIELD(action->type, Return) ? CHEAT_RETURN_VALUE : selection + 1);

			action->flags |= kActionFlag_PrefillDone;
		}
	}

	if(EXTRACT_FIELD(action->type, DelayEnable))
	{
		if(TEST_FIELD(action->type, OneShot) && TEST_FIELD(action->type, RestoreValue))
		{
			/* Keep */
			executeOperation = 1;

			if(action->frameTimer++ >= EXTRACT_FIELD(action->type, DelayEnable) * ATTOSECONDS_TO_HZ(video_screen_get_frame_period(machine->primary_screen).attoseconds))
				action->flags |= kActionFlag_OperationDone;
		}
		else
		{
			if(action->frameTimer++ >= EXTRACT_FIELD(action->type, DelayEnable) * ATTOSECONDS_TO_HZ(video_screen_get_frame_period(machine->primary_screen).attoseconds))
			{
				/* Delay */
				executeOperation = 1;
				action->frameTimer = 0;

				if(TEST_FIELD(action->type, OneShot))
					action->flags |= kActionFlag_OperationDone;
			}
		}
	}
	else
	{
		if(action->frameTimer++)
		{
			if(action->frameTimer >= EXTRACT_FIELD(action->type, DelayEnable) * ATTOSECONDS_TO_HZ(video_screen_get_frame_period(machine->primary_screen).attoseconds))
				action->frameTimer = 0;

			return (TEST_FIELD(action->type, Return) ? CHEAT_RETURN_VALUE : selection + 1);
		}
		else
		{
			executeOperation = 1;

			action->flags |= kActionFlag_OperationDone;
		}
	}

	if(executeOperation)
	{
		/* do cheat operation */
		switch(EXTRACT_FIELD(action->type, CodeType))
		{
			case kCodeType_Write:
			case kCodeType_PDWWrite:
			case kCodeType_RWrite:
			case kCodeType_IWrite:
			case kCodeType_VWrite:
			case kCodeType_VRWrite:
			case kCodeType_Move:
				cheat_periodicOperation(action);
				break;

			case kCodeType_CWrite:
			case kCodeType_Popup:
				if(cheat_periodicCondition(action))
					cheat_periodicOperation(action);
				break;

			case kCodeType_Branch:
				if(cheat_periodicCondition(action))
					return action->extendData;
				break;

			case kCodeType_Loop:
				{
					int counter = ReadData(action);

					if(counter)
					{
						WriteData(action, counter - 1);

						return (TEST_FIELD(action->type, DataRead) ? cheatVariable[action->data] : action->data);
					}
				}
				break;
		}
	}

	return (TEST_FIELD(action->type, Return) ? CHEAT_RETURN_VALUE : selection + 1);
}

/*-----------------------------------------------------
  cheat_periodicEntry - management for cheat entries
-----------------------------------------------------*/

static void cheat_periodicEntry(running_machine *machine, CheatEntry * entry)
{
	int		i			= 0;
	UINT8	pressedKey	= 0;
	UINT8	isSelect	= 0;

	/* ***** 1st, handle activation key ***** */

	if(!ui_is_menu_active())
	{
		/* NOTE : activation key should be not checked in activating UI menu because it conflicts activation key checker */
		if((entry->flags & kCheatFlag_HasActivationKey1) && input_code_pressed_once(entry->activationKey1))
			pressedKey = 1;
		else if((entry->flags & kCheatFlag_HasActivationKey2) && input_code_pressed_once(entry->activationKey2))
			pressedKey = 2;
	}

	if(entry->flags & kCheatFlag_Select)
	{
		if(pressedKey)
		{
			if(pressedKey == 1)
			{
				if(--entry->selection < 0)
					entry->selection = entry->labelIndexLength - 1;
			}
			else
			{
				if(++entry->selection >= entry->labelIndexLength)
					entry->selection = 0;
			}

			/* NOTE : in handling activatio key, forced to activate a cheat even if one shot */
			if(!(entry->flags & kCheatFlag_OneShot) && !(entry->labelIndex[entry->selection]))
				DeactivateCheat(entry);
			else
				ActivateCheat(entry);

			if(TEST_FIELD(cheatOptions, ActivationKeyMessage))
			{
				if(!entry->selection)
					ui_popup_time(1,"%s disabled", entry->name);
				else
					ui_popup_time(1,"%s : %s selected", entry->name, entry->actionList[entry->labelIndex[entry->selection]].optionalName);
			}
		}
	}
	else
	{
		/* NOTE : if value-selection has activatinon key, no handling -----*/
		if(!(entry->flags & kCheatFlag_UserSelect) && pressedKey)
		{
			if(entry->flags & kCheatFlag_OneShot)
			{
				ActivateCheat(entry);

				if(TEST_FIELD(cheatOptions, ActivationKeyMessage))
					ui_popup_time(1,"set %s", entry->name);
			}
			else if(entry->flags & kCheatFlag_Active)
			{
				DeactivateCheat(entry);

				if(TEST_FIELD(cheatOptions, ActivationKeyMessage))
					ui_popup_time(1,"%s disabled", entry->name);
			}
			else
			{
				ActivateCheat(entry);

				if(TEST_FIELD(cheatOptions, ActivationKeyMessage))
					ui_popup_time(1,"%s enabled", entry->name);
			}
		}
	}

	/* ***** 2nd, do cheat actions ***** */

	/* if "OFF", no action */
	if(!(entry->flags & kCheatFlag_Active))
		return;

	while(1)
	{
		if(entry->actionList[i].flags & kActionFlag_Custom)
		{
			if(EXTRACT_FIELD(entry->actionList[i].type, CodeType) == kCustomCode_LabelSelect)
			{
				i = entry->labelIndex[entry->selection];
				isSelect = 1;
				continue;
			}
			else
				i++;
		}
		else if((entry->actionList[i].flags & kActionFlag_OperationDone) || (entry->actionList[i].flags & kActionFlag_NoAction))
		{
			i++;
		}
		else if(entry->actionList[i].flags & kActionFlag_OldFormat)
		{
			int	tempType	= entry->actionList[i].type;
			int	j			= i;

			entry->actionList[i].type = ConvertToNewCode(&entry->actionList[i]);
			i = cheat_periodicAction(machine, &entry->actionList[i], i);
			entry->actionList[j].type = tempType;
		}
		else
			i = cheat_periodicAction(machine, &entry->actionList[i], i);

		if(i >= entry->actionListLength)
			break;

		if(isSelect && EXTRACT_FIELD(entry->actionList[i].type, Link) != kLink_SubLink)
			break;
	}

	/* ***** 3rd, deactive a cheat if one shot ***** */

	if(entry->flags & kCheatFlag_OneShot)
	{
		UINT8 done = 1;

		i = 0;
		isSelect = 0;

		while(1)
		{
			if(entry->actionList[i].flags & kActionFlag_Custom)
			{
				if(EXTRACT_FIELD(entry->actionList[i].type, CodeType) == kCustomCode_LabelSelect)
				{
					i = entry->labelIndex[entry->selection];
					isSelect = 1;
					continue;
				}
			}
			else if(entry->actionList[i].flags & kActionFlag_NoAction)
			{
				i++;
			}
			else if(!(entry->actionList[i].flags & kActionFlag_OperationDone))
			{
				done = 0;
				i++;
			}
			else
				i++;

			if(i >= entry->actionListLength)
				break;

			if(isSelect && EXTRACT_FIELD(entry->actionList[i].type, Link) != kLink_SubLink)
				break;
		}

		if(done)
			DeactivateCheat(entry);
	}
}

/*------------------------------------------------------------------
  UpdateAllCheatInfo - update all cheat info when database loaded
------------------------------------------------------------------*/

static void UpdateAllCheatInfo(void)
{
	int i;

	/* update flags for all CheatEntry */
	for(i = 0; i < cheatListLength; i++)
	{
		UpdateCheatInfo(&cheatList[i], 1);

		if(cheatList[i].flags & kCheatFlag_Select)
			BuildLabelIndexTable(&cheatList[i]);
	}

	SetLayerIndex();
}

/*---------------------------------------------------------------------------------------------
  UpdateCheatInfo - check several fields on CheatEntry and CheatAction then set flags
                    "isLoadTime" parameter is set when called UpdateAllCheatInfo() right now
---------------------------------------------------------------------------------------------*/

static void UpdateCheatInfo(CheatEntry * entry, UINT8 isLoadTime)
{
	int		i;
	int		flags = 0;
	UINT8	isOneShot = 1;
	UINT8	isNull = 1;
	UINT8	isSeparator = 1;
	UINT8	isNewFormat = 1;

	flags = entry->flags & kCheatFlag_PersistentMask;

	for(i = 0; i < entry->actionListLength; i++)
	{
		CheatAction * action = &entry->actionList[i];

		int actionFlags = action->flags & kActionFlag_PersistentMask;

		action->region = EXTRACT_FIELD(action->type, CPURegion);

		if(actionFlags & kActionFlag_OldFormat)
		{
			if(	(EXTRACT_FIELD(action->type, LocationType) == kLocation_Custom) &&
				(EXTRACT_FIELD(action->type, LocationParameter) == kCustomLocation_Select))
					flags |= kCheatFlag_Select;

			if(EXTRACT_FIELD(action->type, LocationType) != kLocation_Custom)
				isNull = 0;

			if(!TEST_FIELD(action->type, OneShot))
				isOneShot = 0;

			if(TEST_FIELD(action->type, UserSelectEnable))
				flags |= kCheatFlag_UserSelect;

			if(	(EXTRACT_FIELD(action->type, LocationType) == kLocation_Custom) &&
				(EXTRACT_FIELD(action->type, LocationParameter) == kCustomLocation_Select))
					actionFlags |= kActionFlag_NoAction;

			if(	(EXTRACT_FIELD(action->type, LocationType) == kLocation_IndirectIndexed) ||
				((EXTRACT_FIELD(action->type, LocationType) != kLocation_Custom) &&
				((EXTRACT_FIELD(action->type, Operation) | EXTRACT_FIELD(action->type, OperationExtend) << 2) == 1)))
					actionFlags |= kActionFlag_IndexAddress;

			if(i && entry->actionList[i-1].type == entry->actionList[i].type)
			{
				if(	entry->actionList[i].address >= entry->actionList[i-1].address &&
					entry->actionList[i].address <= entry->actionList[i-1].address + 3)
						actionFlags |= kActionFlag_NoAction;
			}

			isNewFormat = 0;
		}
		else
		{
			if(action->region != CUSTOM_CODE)
			{
				switch(EXTRACT_FIELD(action->type, CodeType))
				{
					case kCodeType_PDWWrite:
						actionFlags |= kActionFlag_PDWWrite;
						break;

					case kCodeType_RWrite:
						actionFlags |= kActionFlag_Repeat;
						break;

					case kCodeType_IWrite:
					case kCodeType_Move:
					case kCodeType_Loop:
						if(action->extendData != ~0)
							actionFlags |= kActionFlag_IndexAddress;
						break;

					case kCodeType_CWrite:
					case kCodeType_Branch:
						actionFlags |= kActionFlag_CheckCondition;
						break;

					case kCodeType_VWrite:
						flags |= kCheatFlag_UserSelect;

						if(action->extendData != ~0)
							actionFlags |= kActionFlag_IndexAddress;
						break;

					case kCodeType_VRWrite:
						flags |= kCheatFlag_UserSelect;
						actionFlags |= kActionFlag_Repeat;
						break;

					case kCodeType_Popup:
						actionFlags |= kActionFlag_CheckCondition;

						if(action->extendData != ~0)
							actionFlags |= kActionFlag_IndexAddress;
						break;

					case kCodeType_Watch:
						actionFlags |= kActionFlag_NoAction;
						break;
				}

				isNull = 0;
				isSeparator = 0;
			}
			else
			{
				/* is label-select? */
				if(EXTRACT_FIELD(action->type, CodeType) == kCustomCode_LabelSelect)
					flags |= kCheatFlag_Select;

				/* is comment? */
				if(EXTRACT_FIELD(action->type, CodeType) != kCustomCode_Comment)
					isNull = 0;

				/* is separator? */
				if(EXTRACT_FIELD(action->type, CodeType) != kCustomCode_Separator)
					isSeparator = 0;

				/* is layer index? */
				if(EXTRACT_FIELD(action->type, CodeType) == kCustomCode_Layer)
					flags |= kCheatFlag_LayerIndex;

				actionFlags |= kActionFlag_NoAction;
				actionFlags |= kActionFlag_Custom;
			}

			/* is one shot? */
			if(!TEST_FIELD(action->type, OneShot))
				isOneShot = 0;
		}

		action->flags = actionFlags;
	}

	/* one shot is set if "ALL" actions are not an one shot */
	if(isOneShot)
		flags |= kCheatFlag_OneShot;

	if(isNull)
	{
		if(entry->actionListLength > 1)
			flags |= kCheatFlag_ExtendComment;
		else
			flags |= kCheatFlag_Null;
	}

	if(isSeparator)
	{
		flags |= kCheatFlag_Null;
		flags |= kCheatFlag_Separator;
	}

	if(isNewFormat == 0)
		flags |= kCheatFlag_OldFormat;

	entry->flags = (flags & kCheatFlag_InfoMask) | (entry->flags & ~kCheatFlag_InfoMask);

	/* clear dirty flag as "this entry has already exist in the database" when loaded from a database */
	if(isLoadTime)
		entry->flags &= ~kCheatFlag_Dirty;

	CheckCodeFormat(entry);
}

/*--------------------
  AnalyseCodeFormat
--------------------*/

static UINT32 AnalyseCodeFormat(CheatEntry * entry, CheatAction * action)
{
	UINT32 errorFlag = 0;

	/* check type field */
	if(action->flags & kActionFlag_OldFormat)
	{
		UINT8	type		= EXTRACT_FIELD(action->type, LocationType);
		UINT8	parameter	= EXTRACT_FIELD(action->type, LocationParameter);
		UINT8	operation	= EXTRACT_FIELD(action->type, Operation);

		if(type >= kLocation_Unused5)
			errorFlag |= kErrorFlag_InvalidLocationType;

		if(TEST_FIELD(action->type, OperationExtend))
			errorFlag |= kErrorFlag_InvalidOperation;

		if(type == kLocation_IndirectIndexed)
		{
			if(operation != kOperation_WriteMask && operation != kOperation_SetOrClearBits)
				errorFlag |= kErrorFlag_ConflictedExtendField;
		}

		if(operation == kOperation_ForceRange)
		{
			if(EXTRACT_FIELD(action->type, MSB16) <= EXTRACT_FIELD(action->type, LSB16))
				errorFlag |= kErrorFlag_InvalidRange;
		}

		if(type == kLocation_MemoryRegion)
		{
			if(!TEST_FIELD(action->type, RestorePreviousValue))
				errorFlag |= kErrorFlag_NoRestorePreviousValue;
		}

		if((entry->flags & kCheatFlag_UserSelect) && (entry->flags & kCheatFlag_Select))
			errorFlag |= kErrorFlag_ConflictedSelects;

		if(TEST_FIELD(action->type, UserSelectEnable))
		{
			if(!action->originalDataField)
				errorFlag |= kErrorFlag_NoSelectableValue;
		}

		if(type == kLocation_Custom && parameter == kCustomLocation_Select)
		{
			if(entry->actionListLength == 1)
				errorFlag |= kErrorFlag_NoLabel;

			if(action->originalDataField)
				errorFlag |= kErrorFlag_InvalidDataField;

			if(action->extendData)
				errorFlag |= kErrorFlag_InvalidExtendDataField;
		}

		if(operation == kOperation_WriteMask)
		{
			if(type == kLocation_Standard)
			{
				if(!action->extendData || action->extendData == 0xFF || action->extendData == 0xFFFF || action->extendData == 0xFFFFFF)
					errorFlag |= kErrorFlag_InvalidExtendDataField;
			}
			else if(type == kLocation_MemoryRegion)
			{
				if(action->extendData != ~0)
					errorFlag |= kErrorFlag_InvalidExtendDataField;
			}
		}
	}
	else
	{
		if((action->flags & kActionFlag_Custom) == 0)
		{
			UINT8	codeType		= EXTRACT_FIELD(action->type, CodeType);
			UINT8	codeParameter	= EXTRACT_FIELD(action->type, CodeParameter);

			if(codeType >= kCodeType_Unused1 && codeType <= kCodeType_Unused4)
				errorFlag |= kErrorFlag_InvalidCodeType;

			if(action->flags & kActionFlag_CheckCondition)
			{
				if(codeParameter >= kCondition_Unused1 && codeParameter <= kCondition_Unused4)
					errorFlag |= kErrorFlag_InvalidCondition;
			}

			if(codeType == kCodeType_PDWWrite)
			{
				if(EXTRACT_FIELD(action->type, AddressRead) == kReadFrom_Variable)
					errorFlag |= kErrorFlag_ForbittenVariable;
			}

			if(EXTRACT_FIELD(action->type, Link) == kLink_CopyLink)
			{
				if(!entry->flags & kCheatFlag_UserSelect)
					errorFlag |= kErrorFlag_CopyLinkWithoutSelect;
			}

			if(	TEST_FIELD(action->region, RegionFlag) ||
				EXTRACT_FIELD(action->type, AddressSpace) == kAddressSpace_OpcodeRAM)
			{
				if(!TEST_FIELD(action->type, RestoreValue))
					errorFlag |= kErrorFlag_NoRestorePreviousValue;
			}

			if(	EXTRACT_FIELD(action->type, CodeType) == kCodeType_VWrite ||
				EXTRACT_FIELD(action->type, CodeType) == kCodeType_VRWrite)
			{
				if(!action->originalDataField)
					errorFlag |= kErrorFlag_NoSelectableValue;
			}

			if(action->flags & kActionFlag_Repeat)
			{
				if(!EXTRACT_FIELD(action->extendData, LSB16))
					errorFlag |= kErrorFlag_NoRepeatCount;
			}

			if(EXTRACT_FIELD(action->type, AddressRead))
			{
				if(EXTRACT_FIELD(action->type, AddressRead) > kReadFrom_Variable)
					errorFlag |= kErrorFlag_UndefinedAddressRead;

				if(action->address >= VARIABLE_MAX_ARRAY)
					errorFlag |= kErrorFlag_AddressVariableOutRange;
			}

			if(TEST_FIELD(action->type, DataRead))
			{
				if(action->data >= VARIABLE_MAX_ARRAY)
					errorFlag |= kErrorFlag_DataVariableOutRange;
			}

			if(codeType == kCodeType_Write)
			{
				if(!action->extendData || action->extendData == 0xFF || action->extendData == 0xFFFF || action->extendData == 0xFFFFFF)
					errorFlag |= kErrorFlag_InvalidExtendDataField;
			}

			if(!(action->flags & kActionFlag_Custom))
			{
				if(TEST_FIELD(action->region, RegionFlag))
				{
					UINT8 region = EXTRACT_FIELD(action->region, RegionIndex);

					if(region >= REGION_MAX)				errorFlag |= kErrorFlag_OutOfCPURegion;
					else if(!region_info_list[region].type)	errorFlag |= kErrorFlag_InvalidCPURegion;
					else if(!IsAddressInRange(action, memory_region_length(action->region)))
															errorFlag |= kErrorFlag_RegionOutOfRange;
				}
				else
				{
					UINT8 cpu = EXTRACT_FIELD(action->region, CPUIndex);

					if(!VALID_CPU(cpu))
						errorFlag |= kErrorFlag_OutOfCPURegion;
					else if(cpu_info_list[cpu].type == 0)
						errorFlag |= kErrorFlag_InvalidCPURegion;

					if(EXTRACT_FIELD(action->type, AddressSpace) > kAddressSpace_EEPROM)
						errorFlag |= kErrorFlag_InvalidAddressSpace;
				}
			}
		}
		else
		{
			if(EXTRACT_FIELD(action->type, CodeParameter) >= kCustomCode_Unused1)
				errorFlag |= kErrorFlag_InvalidCustomCode;
		}
	}

	return errorFlag;
}

/*----------------------------------------
  CheckCodeFormat - code format checker
----------------------------------------*/

static void CheckCodeFormat(CheatEntry * entry)
{
	int		i;
	UINT8	isError = 0;

	for(i = 0; i < entry->actionListLength; i++)
	{
		CheatAction * action = &entry->actionList[i];

		if(AnalyseCodeFormat(entry, action))
			isError = 1;
	}

	if(isError)
	{
		entry->flags |= kCheatFlag_HasWrongCode;
		messageType = kCheatMessage_WrongCode;
		messageTimer = DEFAULT_MESSAGE_TIME;
	}
}

/*-------------------------------------------------------------------------------------------------
  BuildLabelIndexTable - create index table for label-selection and calculate index table length
-------------------------------------------------------------------------------------------------*/

static void BuildLabelIndexTable(CheatEntry *entry)
{
	int i, j, length = 0;

	entry->labelIndexLength = 0;

	for(i = 0, j = 0; i < entry->actionListLength; i++)
	{
		CheatAction * action = &entry->actionList[i];

		if((action->flags && kActionFlag_Custom) && EXTRACT_FIELD(action->type, CodeType) == kCustomCode_LabelSelect)
		{
			entry->labelIndex = malloc_or_die(sizeof(entry->labelIndex) * (++entry->labelIndexLength + 1));
			entry->labelIndex[j++] = i;

			length = action->data ? action->data + i : entry->actionListLength;
		}
		else if(EXTRACT_FIELD(action->type, Link) == kLink_Link)
		{
			if(length)
			{
				entry->labelIndex = realloc(entry->labelIndex, sizeof(entry->labelIndex) * (++entry->labelIndexLength + 1));
				entry->labelIndex[j++] = i;
			}
		}
	}

	if(entry->labelIndexLength <= 1)
	{
		logerror("cheat - BuildLabelIndexTable : %s fails to build due to invalid or no link\n", entry->name);
		return;
	}

	if(entry->flags & kCheatFlag_OneShot)
		entry->selection = 1;

	//logerror("Cheat - Finish building index table for %s (length = %x)\n", entry->name, entry->labelIndexLength);
	//for(i = 0; i < entry->labelIndexLength; i++)
	//  logerror("IndexTable[%x] = %x\n",i,entry->labelIndex[i]);

}

/*----------------
  SetLayerIndex
----------------*/

static void SetLayerIndex(void)
{
	int i, j;

	for(i = 0; i < cheatListLength; i++)
	{
		CheatEntry * entry = &cheatList[i];

		if(entry->flags & kCheatFlag_LayerIndex)
		{
			int length = entry->actionList[0].extendData;

			if(length < cheatListLength && (i + length) <= cheatListLength)
			{
				entry->layerIndex = entry->actionList[0].address;

				for(j = i + 1; j <= i + length; j++)
				{
					CheatEntry * traverse = &cheatList[j];

					if(!(traverse->flags & kCheatFlag_LayerIndex))
						traverse->layerIndex = entry->actionList[0].data;
				}
			}
		}
	}
}

/*-----------------------------------------------------------------------------------------
  DisplayCheatMessage - display cheat message via ui_draw_text_box instead of popup menu
                        warning message is displayed with red back color
-----------------------------------------------------------------------------------------*/

static void DisplayCheatMessage(void)
{
	static const char *const kCheatMessageTable[] = {
		"INVALID MESSAGE!",				// this message is unused
		"cheat option reloaded",
		"cheat code reloaded",
		"user defined search region reloaded",
		"failed to load database!",
		"cheats found",
		"1 result found, added to list",
		"succeeded to save",
		"failed to save!",
		"cheats saved",
		"activation key saved",
		"no activation key!",
		"pre-enable saved",
		"succeeded to add",
		"failed to add!",
		"succeeded to delete",
		"failed to delete!",
		"no search region!",
		"values restored",
		"there are no old values!",
		"saved all memory regions",
		"region invalidated remains results are",
		"debug mode only!",
		"found wrong code!" };

	char buf[64];

	if(!messageType)
		return;

	switch(messageType)
	{
		/* simple message */
		default:
			sprintf(buf, "%s", kCheatMessageTable[messageType]);
			break;

		/* message with data */
		case kCheatMessage_CheatFound:
		case kCheatMessage_InvalidatedRegion:
		{
			search_info *search = get_current_search();

			sprintf(buf, "%d %s", search->num_results, kCheatMessageTable[messageType]);
		}
		break;

		case kCheatMessage_AllCheatSaved:
			sprintf(buf, "%d %s", cheatListLength, kCheatMessageTable[messageType]);
			break;
	}

	/* draw it */
	switch(messageType)
	{
		default:
			ui_draw_text_box(buf, JUSTIFY_CENTER, 0.5f, 0.9f, UI_FILLCOLOR);
			break;

		case kCheatMessage_None:
		case kCheatMessage_FailedToLoadDatabase:
		case kCheatMessage_FailedToSave:
		case kCheatMessage_NoActivationKey:
		case kCheatMessage_FailedToAdd:
		case kCheatMessage_FailedToDelete:
		case kCheatMessage_NoSearchRegion:
		case kCheatMessage_NoOldValue:
		case kCheatMessage_Max:
		case kCheatMessage_DebugOnly:
		case kCheatMessage_WrongCode:
			/* warning message has red background color */
			ui_draw_text_box(buf, JUSTIFY_CENTER, 0.5f, 0.9f, UI_REDCOLOR);
			break;
	}

	/* decrement message timer */
	if(--messageTimer == 0)
		messageType = 0;
}

/*---------------------
  get_address_length
---------------------*/

static UINT8 get_address_length(UINT8 region)
{
	if(region != CUSTOM_CODE)
	{
		cpu_region_info *info = get_cpu_or_region_info(region);

		if(info && info->type)
			return info->address_chars_needed;
	}

	return 8;
}

/*------------------
  get_region_name
------------------*/

static char *get_region_name(UINT8 region)
{
	if(TEST_FIELD(region, RegionIndex))
	{
		if(region >= REGION_INVALID && region < REGION_MAX)
			return (char *)kRegionNames[EXTRACT_FIELD(region, RegionIndex)];
	}
	else
	{
		if(VALID_CPU(region))
			return (char *)kRegionNames[EXTRACT_FIELD(region, CPUIndex) + 1];
	}

	return (char *)kRegionNames[0];
}

/*------------------------------------------------------------------------------------
  build_cpu_region_info_list - get CPU and region info when initialize cheat system
------------------------------------------------------------------------------------*/

static void build_cpu_region_info_list(running_machine *machine)
{
	int i;

	/* do regions */
	{
		const rom_entry *traverse = rom_first_region(machine->gamedrv);

		memset(region_info_list, 0, sizeof(cpu_region_info) * REGION_LIST_LENGTH);

		while(traverse)
		{
			if(ROMENTRY_ISREGION(traverse))
			{
				UINT8 region_type = ROMREGION_GETTYPE(traverse);

				/* non-cpu region */
				if(region_type >= REGION_GFX1 && region_type <= REGION_PLDS)
				{
					UINT8	bit_state		= 0;
					UINT32	length			= memory_region_length(region_type);
					cpu_region_info *info	= &region_info_list[region_type - REGION_INVALID];

					info->type						= region_type;
					info->data_bits					= ROMREGION_GETWIDTH(traverse);
					info->address_bits				= 0;
					info->address_mask				= length;
					info->address_chars_needed		= info->address_bits >> 2;
					info->endianness				= ROMREGION_ISBIGENDIAN(traverse);

					if(info->address_bits & 3)
						info->address_chars_needed++;

					/* build address mask */
					for(i = 0; i < 32; i++)
					{
						UINT32 mask = 1 << (31 - i);

						if(bit_state)
							info->address_mask |= mask;
						else
						{
							if(info->address_mask & mask)
							{
								info->address_bits = 32 - i;
								bit_state = 1;
							}
						}
					}
				}
			}

			traverse = rom_next_region(traverse);
		}
	}

	/* do CPUs */
	{
		memset(cpu_info_list, 0, sizeof(cpu_region_info) * MAX_CPU);

		for(i = 0; i < cpu_gettotalcpu(); i++)
		{
			cpu_region_info	*cpu_info		= &cpu_info_list[i];
			cpu_region_info	*region_info	= &region_info_list[REGION_CPU1 + i - REGION_INVALID];
			cpu_type		type			= machine->config->cpu[i].type;

			cpu_info->type						= type;
			cpu_info->data_bits					= cputype_databus_width(type, ADDRESS_SPACE_PROGRAM);
			cpu_info->address_bits				= cputype_addrbus_width(type, ADDRESS_SPACE_PROGRAM);
			cpu_info->address_mask				= 0xFFFFFFFF >> (32 - cpu_info->address_bits);
			cpu_info->address_chars_needed		= cpu_info->address_bits >> 2;
			cpu_info->endianness				= (cputype_endianness(type) == CPU_IS_BE);
			cpu_info->address_shift				= cputype_addrbus_shift(type, ADDRESS_SPACE_PROGRAM);

			if(cpu_info->address_bits & 0x3)
				cpu_info->address_chars_needed++;

			/* copy CPU info to region info */
			memcpy(region_info, cpu_info, sizeof(cpu_region_info));
		}
	}
}
