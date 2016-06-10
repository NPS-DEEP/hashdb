# Author:  Bruce Allen
# Created: 2/25/2013
#
# The software provided here is released by the Naval Postgraduate
# School, an agency of the U.S. Department of Navy.  The software
# bears no warranty, either expressed or implied. NPS does not assume
# legal liability nor responsibility for a User's use of the software
# or the results of such use.
#
# Please note that within the United States, copyright protection,
# under Section 105 of the United States Code, Title 17, is not
# available for any work of the United States Government and/or for
# any works created by United States Government employees. User
# acknowledges that this software contains work which was created by
# NPS government employees and is therefore in the public domain and
# not subject to copyright.
#
# Released into the public domain on February 25, 2013 by Bruce Allen.

# This file assists in testing and timing analysis.

python_tests = \
	new_database.py \
	import_export.py \
	database_manipulation.py \
	scan.py \
	statistics.py \
	performance_analysis.py \
	json_modes.py

EXTRA_DIST = \
	$(python_tests) \
	helpers.py \
	plot_performance.py \
	performance_report.tex \
	plot_timestamp.py

if HAVE_PYTHON
  TESTS = $(python_tests)
endif

performance_report:
	./plot_performance.py
	pdflatex performance_report.tex

clean-local:
	rm -rf temp_* performance_report.aux performance_report.log \
	performance_report.out performance_report.pdf

