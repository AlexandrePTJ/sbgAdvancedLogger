// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Alexandre Petitjean <alpetitjean@gmail.com>
#pragma once

// STL headers
#include <array>
#include <optional>

// 3rdparty headers
#include <sbgECom.h>

// Local headers
#include "sbgEComAnalyzer.h"

namespace sbg::advlogger
{
	class CSbgEComAnalyzerPrivate
	{
	public:
		//---------------------------------------------------------------------//
		//- Constructor / Destructor                                          -//
		//---------------------------------------------------------------------//
		CSbgEComAnalyzerPrivate(const CSbgEComAnalyzer *pParent);
		~CSbgEComAnalyzerPrivate();

		//---------------------------------------------------------------------//
		//- Operations                                                        -//
		//---------------------------------------------------------------------//
		void         initialize();
		void         process(std::span<const uint8_t> data);
		SbgErrorCode processMessage(SbgEComClass msgClass, SbgEComMsgId msg, const SbgEComLogUnion *pLogData);
		void         processTimeReference(const SbgEComLogUtc &utcTimeReference);
		void         processEkvNav(const SbgEComLogEkfNav &ekfNav)const;
		void         processGnssPos(const SbgEComLogGnssPos &gnssPos, uint8_t index);

		//---------------------------------------------------------------------//
		//- Data                                                              -//
		//---------------------------------------------------------------------//
		const CSbgEComAnalyzer *m_pParent;

		SbgStreamBuffer m_buffer;
		SbgInterface    m_interface;
		SbgEComHandle   m_parserHandle;

		std::optional<SbgEComLogUtc>                    m_lastUTCTimeReference;
		std::array<std::optional<SbgEComLogGnssPos>, 2> m_lastGnssPos;
	};
}// namespace sbg::advlogger
