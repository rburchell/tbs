/*
 * Copyright (c) 2014 Robin Burchell <robin+git@viroteck.net>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <assert.h>

#include "target.h"
#include "futils.h"

translation_unit::translation_unit(const std::string &path)
{
    m_path = path;
}

std::string translation_unit::path() const
{
    //printf("path %s\n", m_path.substr(0, m_path.find_last_of("/")).c_str());
    return m_path.substr(0, m_path.find_last_of("/"));
}

std::string translation_unit::source_name() const
{
    return futils::basename(m_path.c_str());
}

std::string translation_unit::object_name() const
{
    //printf("for %s\n", m_path.c_str());
    std::string basename = futils::basename(m_path.c_str());
    //printf("got %s\n", basename.c_str());
    return basename.substr(0, basename.find_last_of(".")) + ".o";
}

target::target()
    : m_type(TYPE_APPLICATION)
    , m_explicitly_named(false)
{
}

void target::set_path(const std::string &path)
{
    if (m_name.empty())
        m_name = futils::basename(path.c_str());
    m_path = path;
}

std::string target::path() const
{
    return m_path;
}

void target::set_name(const std::string &name)
{
    m_explicitly_named = true;
    m_name = name;
}

std::string target::name() const
{
    return m_name;
}

bool target::explicitly_named() const
{
    return m_explicitly_named;
}

std::vector<translation_unit> target::translation_units() const
{
    return m_translation_units;
}

void target::set_translation_units(const std::vector<translation_unit> &files)
{
    m_translation_units = files;
}

std::string target::compiler_flags() const
{
    return m_compiler_flags;
}

void target::set_compiler_flags(const std::string &flags)
{
    m_compiler_flags = flags;
}

std::string target::linker_flags() const
{
    return m_linker_flags;
}

void target::set_linker_flags(const std::string &flags)
{
    m_linker_flags = flags;
}


bool target::has_feature(const std::string &feature) const
{
    return m_features.find(feature) != std::string::npos;
}

void target::set_features(const std::string &features)
{
    m_features = features;
}

target::target_type target::type() const
{
    return m_type;
}

void target::set_type(target::target_type type)
{
    m_type = type;
}

