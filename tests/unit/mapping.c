/** Unit tests for sample value mapping.
 *
 * @author Steffen Vogel <stvogel@eonerc.rwth-aachen.de>
 * @copyright 2017, Institute for Automation of Complex Power Systems, EONERC
 *********************************************************************************/

#include <criterion/criterion.h>

#include "mapping.h"

Test(mapping, parse)
{
	int ret;
	struct mapping_entry m;
	
	ret = mapping_entry_parse_str(&m, "ts.origin");
	cr_assert_eq(ret, 0);
	cr_assert_eq(m.type, MAPPING_TYPE_TIMESTAMP);
	cr_assert_eq(m.timestamp.id, MAPPING_TIMESTAMP_ORIGIN);
	
	ret = mapping_entry_parse_str(&m, "hdr.sequence");
	cr_assert_eq(ret, 0);
	cr_assert_eq(m.type, MAPPING_TYPE_HEADER);
	cr_assert_eq(m.header.id, MAPPING_HEADER_SEQUENCE);

	ret = mapping_entry_parse_str(&m, "stats.owd.mean");
	cr_assert_eq(ret, 0);
	cr_assert_eq(m.type, MAPPING_TYPE_STATS);
	cr_assert_eq(m.stats.id, STATS_OWD);
	cr_assert_eq(m.stats.type, MAPPING_STATS_TYPE_MEAN);
	
	ret = mapping_entry_parse_str(&m, "data[1-2]");
	cr_assert_eq(ret, 0);
	cr_assert_eq(m.type, MAPPING_TYPE_DATA);
	cr_assert_eq(m.data.offset, 1);
	cr_assert_eq(m.length, 2);

	ret = mapping_entry_parse_str(&m, "data[5-5]");
	cr_assert_eq(ret, 0);
	cr_assert_eq(m.type, MAPPING_TYPE_DATA);
	cr_assert_eq(m.data.offset, 5);
	cr_assert_eq(m.length, 1);

	ret = mapping_entry_parse_str(&m, "data[22]");
	cr_assert_eq(ret, 0);
	cr_assert_eq(m.type, MAPPING_TYPE_DATA);
	cr_assert_eq(m.data.offset, 22);
	cr_assert_eq(m.length, 1);
	
	ret = mapping_entry_parse_str(&m, "data[]");
	cr_assert_neq(ret, 0);

	ret = mapping_entry_parse_str(&m, "data[1.1-2f]");
	cr_assert_neq(ret, 0);

	/* Missing parts */
	ret = mapping_entry_parse_str(&m, "stats.owd");
	cr_assert_neq(ret, 0);
		
	ret = mapping_entry_parse_str(&m, "data");
	cr_assert_neq(ret, 0);

	/* This a type */
	ret = mapping_entry_parse_str(&m, "hdr.sequences");
	cr_assert_neq(ret, 0);

	/* Check for superfluous chars at the end */
	ret = mapping_entry_parse_str(&m, "stats.ts.origin.bla");
	cr_assert_neq(ret, 0);
	
	ret = mapping_entry_parse_str(&m, "stats.ts.origin.");
	cr_assert_neq(ret, 0);

	ret = mapping_entry_parse_str(&m, "data[1-2]bla");
	cr_assert_neq(ret, 0);

	/* Negative length of chunk */
	ret = mapping_entry_parse_str(&m, "data[5-3]");
	cr_assert_eq(ret, -1);
}