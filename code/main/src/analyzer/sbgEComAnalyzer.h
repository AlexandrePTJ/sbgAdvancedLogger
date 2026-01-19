// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Alexandre Petitjean <alpetitjean@gmail.com>
#pragma once

// STL headers
#include <memory>

// Project headers
#include <analyzer/abstractFormatAnalyzer.h>

namespace sbg::advlogger
{
	class CSbgEComAnalyzerPrivate;

	class CSbgEComAnalyzer : public CAbstractFormatAnalyzer
	{
	public:
		//---------------------------------------------------------------------//
		//- Constructor / Destructor                                          -//
		//---------------------------------------------------------------------//
		CSbgEComAnalyzer();
		~CSbgEComAnalyzer() override;

		//---------------------------------------------------------------------//
		//- Parameters                                                        -//
		//---------------------------------------------------------------------//
		std::string getId() const override;

		//---------------------------------------------------------------------//
		//- Operations                                                        -//
		//---------------------------------------------------------------------//
		void process(std::span<const uint8_t> data) override;

	private:
		friend class CSbgEComAnalyzerPrivate;
		std::unique_ptr<CSbgEComAnalyzerPrivate> m_pImpl;
	};

}// namespace sbg::advlogger