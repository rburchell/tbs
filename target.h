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

#ifndef TARGET_H
#define TARGET_H

#include <vector>
#include <string>

class translation_unit
{
public:
    translation_unit(const std::string &path);
    std::string path() const;
    std::string source_name() const;
    std::string object_name() const;

private:
    std::string m_path;
};

class target
{
public:
    target();

    void set_path(const std::string &path);
    std::string path() const;

    void set_name(const std::string &name);
    std::string name() const;
    bool explicitly_named() const; /* was set_name invoked? */

    std::vector<translation_unit> translation_units() const;
    void set_translation_units(const std::vector<translation_unit> &files);

    std::string compiler_flags() const;
    void set_compiler_flags(const std::string &flags);

    std::string linker_flags() const;
    void set_linker_flags(const std::string &flags);

    bool has_feature(const std::string &feature) const;
    void set_features(const std::string &features);

    enum target_type {
        TYPE_APPLICATION,
        TYPE_DLL
    };

    target_type type() const;
    void set_type(target_type type);

private:
    std::string m_name;
    target_type m_type;
    bool m_explicitly_named;
    std::string m_path;
    std::string m_compiler_flags;
    std::string m_linker_flags;
    std::string m_features;
    std::vector<translation_unit> m_translation_units;
};

#endif // TARGET_H
