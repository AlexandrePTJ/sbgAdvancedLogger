#include "abstractFormatAnalyzer.h"

using namespace sbg::advlogger;

//---------------------------------------------------------------------//
//- Parameters                                                        -//
//---------------------------------------------------------------------//
void CAbstractFormatAnalyzer::setPositionCallback(PositionCallback positionCb)
{
	m_positionCb = std::move(positionCb);
}
