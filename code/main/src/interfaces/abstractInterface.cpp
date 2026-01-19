#include "abstractInterface.h"

using namespace sbg::advlogger;

//---------------------------------------------------------------------//
//- Constructor / Destructor                                          -//
//---------------------------------------------------------------------//
CAbstractInterface::CAbstractInterface(asio::io_context &ioc):
m_ioc(ioc)
{
}

//---------------------------------------------------------------------//
//- Operations                                                        -//
//---------------------------------------------------------------------//
bool CAbstractInterface::isActive() const
{
	return m_isActive;
}
