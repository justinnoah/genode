/*
 * \brief  Kernel-specific raw-output back end
 * \author Norman Feske
 * \date   2016-03-08
 */

/*
 * Copyright (C) 2016-2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _INCLUDE__BASE__INTERNAL__RAW_WRITE_STRING_H_
#define _INCLUDE__BASE__INTERNAL__RAW_WRITE_STRING_H_

/* L4/Fiasco includes */
#include <fiasco/syscall.h>

namespace Genode {

	void raw_write_string(char const *str)
	{
		using namespace Fiasco;
		outstring(const_cast<char *>(str));
	}
}

#endif /* _INCLUDE__BASE__INTERNAL__RAW_WRITE_STRING_H_ */
