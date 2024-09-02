/*
 * CGItable.cpp
 *
 *  Created on: Aug 5, 2024
 *      Author: dig
 */

#include "cgiScripts.h"
#include "CGItable.h"

#include "measureTask.h"
#include "log.h"

// @formatter:off
// do not alter
const tCGI CGIurls[] = {
		{ "/cgi-bin/readvar", (tCGIHandler_t) readCGIvalues, (CGIresponseFileHandler_t) readVarScript },  // !!!!!! index  !!
		{ "/cgi-bin/writevar", (tCGIHandler_t) readCGIvalues, (CGIresponseFileHandler_t) readVarScript },  // !!!!!! index  !!
		{ "/action_page.php", (tCGIHandler_t) readCGIvalues,(CGIresponseFileHandler_t) actionRespScript },
		{ "/cgi-bin/getLogMeasValues", (tCGIHandler_t) readCGIvalues, (CGIresponseFileHandler_t) getDayLogScript},
		{ "/cgi-bin/getRTMeasValues", (tCGIHandler_t) readCGIvalues, (CGIresponseFileHandler_t) getRTMeasValuesScript},
		{ "/cgi-bin/getChargeValues", (tCGIHandler_t) readCGIvalues, (CGIresponseFileHandler_t) getChargeValuesScript},
		{ "/cgi-bin/getInfoValues", (tCGIHandler_t) readCGIvalues, (CGIresponseFileHandler_t) getInfoValuesScript},
		{ "/cgi-bin/getCalValues", (tCGIHandler_t) readCGIvalues, (CGIresponseFileHandler_t) getCalValuesScript},
		{ "/cgi-bin/getSensorName", (tCGIHandler_t) readCGIvalues, (CGIresponseFileHandler_t) getSensorNameScript},
		{ "/cgi-bin/saveSettings", (tCGIHandler_t) readCGIvalues, (CGIresponseFileHandler_t) saveSettingsScript},
		{ "/cgi-bin/cancelSettings", (tCGIHandler_t) readCGIvalues, (CGIresponseFileHandler_t) cancelSettingsScript},
		{ "/cgi-bin/getChargeTable", (tCGIHandler_t) readCGIvalues, (CGIresponseFileHandler_t) getChargeTableScript},
		{ "/cgi-bin/getFunction", (tCGIHandler_t) readCGIvalues, (CGIresponseFileHandler_t) getFunctionScript},
		{ NULL,NULL,NULL} // last
};

