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

#include <vector>

#include <Carbon/Carbon.h>

#include "BBLMInterface.h"
#include "BBXTInterface.h"
#include "BBLMTextIterator.h"

#define LANGUAGE_CODE 'Inf7'

#include <syslog.h> //###

enum
{
	kBBLMRunIsSubstitution =  kBBLMFirstUserRunKind,
	kBBLMRunIsInform6,
};

static void AdjustRange(BBLMParamBlock &params, const BBLMCallbackBlock &callbacks)
{
	bool res;
	syslog(LOG_WARNING, "### AdjustRange");
	
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

	syslog(LOG_WARNING, "### CalculateRuns");

	ch = ' ';
	p += pos;
	
	while (1) {
		lastch = ch;
		ch = *p;
		if (!ch)
			break;
		
		if (ch == '"') {
			if (pos > lastpos) {
				syslog(LOG_WARNING, "### run pos %ld len %ld (code)", lastpos, pos-lastpos);
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
						syslog(LOG_WARNING, "### run pos %ld len %ld (doublequote)", lastpos, pos-lastpos);
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
						syslog(LOG_WARNING, "### run pos %ld len %ld (subst)", lastpos, pos-lastpos);
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
				syslog(LOG_WARNING, "### run pos %ld len %ld (doublequote)", lastpos, pos-lastpos);
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
				syslog(LOG_WARNING, "### run pos %ld len %ld (code)", lastpos, pos-lastpos);
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
				syslog(LOG_WARNING, "### run pos %ld len %ld (comment)", lastpos, pos-lastpos);
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
				syslog(LOG_WARNING, "### run pos %ld len %ld (code)", lastpos, (pos-1)-lastpos);
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
				syslog(LOG_WARNING, "### run pos %ld len %ld (I6)", lastpos, pos-lastpos);
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

#define NUM_HEADINGS (5)

typedef struct heading_def_struct {
	int len;
	char name[8];
} heading_def_t;
static heading_def_t heading_def[NUM_HEADINGS] = {
	{6, "volume"},
	{4, "book"},
	{4, "part"},
	{7, "chapter"},
	{7, "section"}
};


static OSErr ScanForFunctions(BBLMParamBlock &params, const BBLMCallbackBlock &bblm_callbacks)
{
	BBLMTextIterator p(params);
	//bool res;
	UniChar ch, lastch;
	//UInt32 lastpos = 0;
	UInt32 pos = 0;
	
	int pendingheaders[NUM_HEADINGS];
	for (int lx=0; lx<NUM_HEADINGS; lx++)
		pendingheaders[lx] = -1;
	
	std::vector<UniChar> linebuf;
	int wordend = -1;
	UInt32 linestart = 0;
	
	syslog(LOG_WARNING, "### ScanForFunctions");
	
	ch = ' ';
	p += pos;
	
	while (1) {
		lastch = ch;
		ch = *p;
		if (!ch)
			break;
		
		if (ch == '"') {
			p++;
			pos++;
			while ((ch = *p), (ch && ch != '"')) {
				if (ch == '[') {
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
					continue;
				}
				p++;
				pos++;
			}
			if (ch == '"') {
				p++;
				pos++;
			}
			
			ch = ' ';
			continue;
		}
		
		if (ch == '[') {
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
			
			ch = ' ';
			continue;
		}
		
		if (ch == '-' && lastch == '(') {
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
			
			ch = ' ';
			continue;
		}
		
		if (ch == '\n' || ch == '\r') {
			int foundlevel = -1;
			for (int lx=0; lx<NUM_HEADINGS; lx++) {
				if (wordend == heading_def[lx].len) {
					bool match = true;
					for (int ix=0; ix<wordend && match; ix++) {
						if (tolower(linebuf[ix]) != heading_def[lx].name[ix]) {
							match = false;
						}
					}
					if (match) {
						foundlevel = lx;
						break;
					}
				}
			}
			
			if (foundlevel >= 0) {
				BBLMProcInfo procinfo;
				OSErr err;
				
				// Close all the pending functions of this level and lower
				for (int lx=NUM_HEADINGS-1; lx >= foundlevel; lx--) {
					if (pendingheaders[lx] >= 0) {
						int foldstart, foldlen;
						err = bblmGetFunctionEntry(&bblm_callbacks, params.fFcnParams.fFcnList, pendingheaders[lx], procinfo);
						if (err)
							return err;
						procinfo.fFunctionEnd = linestart;
						err = bblmUpdateFunctionEntry(&bblm_callbacks, params.fFcnParams.fFcnList, pendingheaders[lx], procinfo);
						if (err)
							return err;
						foldstart = procinfo.fSelEnd;
						foldlen = (procinfo.fFunctionEnd-1) - foldstart;
						if (foldlen > 0) {
							err = bblmAddFoldRange(&bblm_callbacks, foldstart, foldlen);
							if (err)
								return err;
						}
						
						pendingheaders[lx] = -1;
					}
				}
				
				UniChar *buf = &linebuf[0];
				int linelength = linebuf.size();
				UInt32 offset = 0;
				err = bblmAddTokenToBuffer(&bblm_callbacks, params.fFcnParams.fTokenBuffer, buf, linelength, true, &offset);
				if (err)
					return err;
				bzero(&procinfo, sizeof(procinfo));
				procinfo.fFunctionStart = linestart;
				procinfo.fFunctionEnd = pos;
				procinfo.fSelStart = linestart;
				procinfo.fSelEnd = pos;
				procinfo.fFirstChar = linestart;
				procinfo.fKind = kBBLMFunctionMark;
				procinfo.fIndentLevel = foundlevel;
				procinfo.fFlags = 0;
				procinfo.fNameStart = offset;
				procinfo.fNameLength = linelength;
				
				syslog(LOG_WARNING, "### adding function start %ld, end %ld", linestart, pos);
				UInt32 funcindex = 0;
				err = bblmAddFunctionToList(&bblm_callbacks, params.fFcnParams.fFcnList, procinfo, &funcindex);
				if (err)
					return err;
				
				pendingheaders[foundlevel] = funcindex;
			}
			
			linebuf.clear();
			wordend = -1;
			linestart = pos+1;
		}
		else {
			if (isspace(ch) && linebuf.size() == 0) {
				// ignore leading whitespace in a line
			}
			else {
				if (wordend < 0 && isspace(ch)) {
					// the first non-whitespace character, so it's the end of the first word.
					wordend = linebuf.size();
				}
				linebuf.push_back(ch);
			}
		}
		
		p++;
		pos++;
	}
	
	// Close all the pending functions
	for (int lx=NUM_HEADINGS-1; lx >= 0; lx--) {
		if (pendingheaders[lx] >= 0) {
			BBLMProcInfo procinfo;
			int foldstart, foldlen;
			OSErr err = bblmGetFunctionEntry(&bblm_callbacks, params.fFcnParams.fFcnList, pendingheaders[lx], procinfo);
			if (err)
				return err;
			procinfo.fFunctionEnd = linestart;
			err = bblmUpdateFunctionEntry(&bblm_callbacks, params.fFcnParams.fFcnList, pendingheaders[lx], procinfo);
			if (err)
				return err;
			foldstart = procinfo.fSelEnd;
			foldlen = (procinfo.fFunctionEnd-1) - foldstart;
			if (foldlen > 0) {
				err = bblmAddFoldRange(&bblm_callbacks, foldstart, foldlen);
				if (err)
					return err;
			}
			
			pendingheaders[lx] = -1;
		}
	}
	
	// Should probably adjust the indents so that the minimum is zero. (I.e., if there are no Volumes.)
	
	return noErr;
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
			result = ScanForFunctions(params, bblm_callbacks);
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
