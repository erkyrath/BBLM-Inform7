/*
	BBEdit/TextWrangler Language Module for Inform 7
	https://github.com/erkyrath/BBLM-Inform7
	Created by Andrew Plotkin
 
	This source file is in the public domain.
	The header files in "SDK Headers" are taken from the BBEdit Development Kit, and are copyright Bare Bones Software, Inc.
	http://www.barebones.com/support/develop/
*/

#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include <Carbon/Carbon.h>

#include "BBLMInterface.h"
#include "BBXTInterface.h"
#include "BBLMTextIterator.h"

#define LANGUAGE_CODE 'Inf7'

enum
{
	kBBLMRunIsSubstitution =  kBBLMFirstUserRunKind,
	kBBLMRunIsInform6,
};

static void AdjustRange(BBLMParamBlock &params, const BBLMCallbackBlock &callbacks)
{
	bool res;
	
	/* Run backwards to the most recent code range (not a string or comment). */
	while (params.fAdjustRangeParams.fStartIndex > 0) {
		DescType language;
		BBLMRunCode kind;
		SInt32 pos;
		SInt32 len;
		res = bblmGetRun(&callbacks, params.fAdjustRangeParams.fStartIndex, language, kind, pos, len);
		if (!res)
			return;
		if (kind == kBBLMRunIsCode)
			return;
		params.fAdjustRangeParams.fStartIndex -= 1;
	}
}


static void CalculateRuns(BBLMParamBlock &params, const BBLMCallbackBlock &bblm_callbacks)
{
	BBLMTextIterator p(params);
	bool res;
	UniChar ch, lastch;
	UInt32 lastpos = params.fCalcRunParams.fStartOffset;
	UInt32 pos = lastpos;
	
	lastch = ' ';
	p += pos;
	
	while (1) {
		lastch = ch;
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
				if (ch == '[') {
					if (pos > lastpos) {
						res = bblmAddRun(&bblm_callbacks, LANGUAGE_CODE, kBBLMRunIsDoubleString, lastpos, pos-lastpos);
						if (!res)
							return;
						lastpos = pos;
					}
					p++;
					pos++;
					while (1) {
						ch = *p;
						if (!ch)
							break;
						if (ch == ']' || ch == '"') {
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
						res = bblmAddRun(&bblm_callbacks, LANGUAGE_CODE, kBBLMRunIsSubstitution, lastpos, pos-lastpos);
						if (!res)
							return;
						lastpos = pos;
					}
					continue;
				}
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
			
			ch = ' ';
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
			
			ch = ' ';
			continue;
		}
		
		if (ch == '-' && lastch == '(') {
			if (pos-1 > lastpos) {
				res = bblmAddRun(&bblm_callbacks, LANGUAGE_CODE, kBBLMRunIsCode, lastpos, (pos-1)-lastpos);
				if (!res)
					return;
				lastpos = pos-1;
			}
			
			ch = ' ';
			
			p++;
			pos++;
			while (1) {
				lastch = ch;
				ch = *p;
				if (!ch)
					break;
				if (ch == ')' && lastch == '-') {
					break;
				}
				p++;
				pos++;
			}
			if (ch == ')') {
				p++;
				pos++;
			}
			
			if (pos > lastpos) {
				res = bblmAddRun(&bblm_callbacks, LANGUAGE_CODE, kBBLMRunIsInform6, lastpos, pos-lastpos);
				if (!res)
					return;
				lastpos = pos;
			}
			
			ch = ' ';
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
			AdjustRange(params, bblm_callbacks);
			result = noErr;
			break;
		
		case kBBLMMapRunKindToColorCodeMessage:
			switch (params.fMapRunParams.fRunKind) {
				case kBBLMRunIsSubstitution:
					params.fMapRunParams.fColorCode = kBBLMXMLProcessingInstructionColor;
					params.fMapRunParams.fMapped =	true;
					break;
				case kBBLMRunIsInform6:
					params.fMapRunParams.fColorCode = kBBLMSGMLAttributeValueColor;
					params.fMapRunParams.fMapped =	true;
					break;
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
