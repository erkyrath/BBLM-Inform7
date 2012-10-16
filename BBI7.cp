#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include <Carbon/Carbon.h>

#include "BBLMInterface.h"
#include "BBXTInterface.h"
#include "BBLMTextIterator.h"

#define LANGUAGE_CODE 'Inf7'

static void CalculateRuns(BBLMParamBlock &pb, const BBLMCallbackBlock &bblm_callbacks)
{
	BBLMTextIterator p(pb);
	bool res;
	UniChar ch;
	UInt32 lastpos = pb.fCalcRunParams.fStartOffset;
	UInt32 pos = lastpos;
	
	p += pos;
	
	while (1) {
		ch = *p;
		if (!ch)
			break;
		
		if (ch == '"') {
			if (pos > lastpos) {
				res = bblmAddRun(&bblm_callbacks, LANGUAGE_CODE, kBBLMRunIsCode, lastpos, pos-lastpos);
				if (!res)
					return;
				lastpos = pos;
			}
			
			p++;
			pos++;
			while ((ch = *p), (ch && ch != '"')) {
				p++;
				pos++;
			}
			if (ch == '"') {
				p++;
				pos++;
			}
			
			if (pos > lastpos) {
				res = bblmAddRun(&bblm_callbacks, LANGUAGE_CODE, kBBLMRunIsDoubleString, lastpos, pos-lastpos);
				if (!res)
					return;
				lastpos = pos;
			}
			
			continue;
		}
		
		if (ch == '[') {
			if (pos > lastpos) {
				res = bblmAddRun(&bblm_callbacks, LANGUAGE_CODE, kBBLMRunIsCode, lastpos, pos-lastpos);
				if (!res)
					return;
				lastpos = pos;
			}
			
			int depth = 1;
			
			p++;
			pos++;
			while (1) {
				ch = *p;
				if (!ch)
					break;
				if (ch == '[') {
					depth++;
				}
				if (ch == ']') {
					depth--;
					if (depth <= 0)
						break;
				}
				p++;
				pos++;
			}
			if (ch == ']') {
				p++;
				pos++;
			}
			
			if (pos > lastpos) {
				res = bblmAddRun(&bblm_callbacks, LANGUAGE_CODE, kBBLMRunIsBlockComment, lastpos, pos-lastpos);
				if (!res)
					return;
				lastpos = pos;
			}
			
			continue;
		}
		
		p++;
		pos++;
		
	}
}


extern "C"
{

extern OSErr Inform7MachO(BBLMParamBlock &params,
					  const BBLMCallbackBlock &bblm_callbacks,
						 const BBXTCallbackBlock &bbxt_callbacks);

OSErr Inform7MachO(BBLMParamBlock &params,
			const BBLMCallbackBlock &bblm_callbacks,
			const BBXTCallbackBlock &bbxt_callbacks)
{
	OSErr result;

	if ((params.fSignature != kBBLMParamBlockSignature) ||
		(params.fVersion < kBBLMParamBlockVersion))
	{
		return paramErr;
	}
	
	switch (params.fMessage)
	{
		case kBBLMInitMessage:
		case kBBLMDisposeMessage:
		{
			result = noErr;	// nothing to do
			break;
		}
		
		case kBBLMCalculateRunsMessage:
			CalculateRuns(params, bblm_callbacks);
			result = noErr;
			break;

		case kBBLMScanForFunctionsMessage:
			//ScanForFunctions(params, bblm_callbacks);
			result = noErr;
			break;

		case kBBLMAdjustRangeMessage:
			//AdjustRange(params, bblm_callbacks);
			result = noErr;
			break;
		
		case kBBLMMapRunKindToColorCodeMessage:
			switch (params.fMapRunParams.fRunKind){
			default:
				params.fMapRunParams.fMapped =	false;
			}
			result = noErr;
			break;
		case kBBLMMapColorCodeToColorMessage:
			params.fMapColorParams.fMapped = false;
			result = noErr;
			break;

		case kBBLMEscapeStringMessage:
		case kBBLMAdjustEndMessage:
		case kBBLMSetCategoriesMessage:
		case kBBLMMatchKeywordMessage:
		{
			result = userCanceledErr;
			break;
		}
		
		case kBBLMGuessLanguageMessage:
		{
			result = noErr;
			break;
		}
		
		default:
		{
			result = paramErr;
			break;
		}
	}
	return result;	
}

}
