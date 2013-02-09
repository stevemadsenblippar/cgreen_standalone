#include <cgreen/constraint.h>
#include <cgreen/message_formatting.h>
#include <cgreen/string_comparison.h>
#include <inttypes.h>
#ifndef __cplusplus
#include <stdbool.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _MSC_VER
#include "wincompat.h"
#endif

#ifdef __cplusplus
namespace cgreen {
#endif

static int find_index_of_difference(char *expected, char *actual, size_t size_to_compare) {
    char *expectedp = expected;
    char *actualp = actual;

    while (size_to_compare--) {
        if (*expectedp++ != *actualp++) {
            return actualp - actual;
        }
    }

    return -1;
}

static bool actual_value_not_necessary_for(Constraint *constraint, const char *actual_string, const char *actual_value_string) {
    (void)constraint; // UNUSED!
    return strings_are_equal(actual_string, actual_value_string) ||
            strings_are_equal(actual_string, "true") ||
            strings_are_equal(actual_string, "false");
}


bool parameters_are_not_valid_for(Constraint *constraint, intptr_t actual) {
    char *message = validation_failure_message_for(constraint, actual);

    bool not_valid = (strlen(message) > 0);

    free(message);

    return not_valid;
}

char *validation_failure_message_for(Constraint *constraint, intptr_t actual) {
    const char *name_has_incorrect_size_message =
    		"\t\tWanted to compare contents with [%s], but [%lu] was given for the comparison size.\n";
    const char *null_used_for_compare_message =
            "\t\tWanted to compare contents with [%s], but NULL was used for the pointer we wanted to compare to.\n"
            "\t\tIf you want to explicitly check for null, use the is_null constraint instead.\n";
    const char *null_used_for_actual_message =
            "\t\tWanted to compare contents of [%s] but it had a value of NULL.\n"
            "\t\tIf you want to explicitly check for null, use the is_null constraint instead.\n";



    size_t message_size = strlen(name_has_incorrect_size_message) +
    		strlen(null_used_for_compare_message) +
    		strlen(null_used_for_actual_message) +
    		512; // just in case

    char *message = (char *)malloc(message_size);
    memset(message, 0, message_size);

//    (void)function; // UNUSED!
//  if (function != NULL && strlen(function) > 0) {
//      snprintf(message, message_size - 1, "\tIn mocked function [%s]:\n", function);
//  } else {
//      snprintf(message, message_size - 1, "\tIn assertion:\n");
//  }

    if (is_content_comparing(constraint)) {
        const char *compared_to_name;
        if (constraint->parameter_name != NULL) {
            compared_to_name = constraint->parameter_name;
        } else {
            compared_to_name = constraint->expected_value_name;
        }

        if (constraint->size_of_expected_value <= 0) {
            snprintf(message + strlen(message), message_size - strlen(message) - 1,
                    name_has_incorrect_size_message,
                    compared_to_name,
                    (unsigned long)constraint->size_of_expected_value);

            return message;
        }

        if ((void *)actual == NULL) {
            snprintf(message + strlen(message), message_size - strlen(message) - 1,
            		null_used_for_compare_message,
                    compared_to_name);

                return message;
        }

        if ((void *)constraint->expected_value == NULL) {
            snprintf(message + strlen(message), message_size - strlen(message) - 1,
            		null_used_for_actual_message,
                    compared_to_name);

                return message;
        }
    }

    return message;
}

char *failure_message_for(Constraint *constraint, const char *actual_string, intptr_t actual) {
    char actual_value_string[32];
    snprintf(actual_value_string, sizeof(actual_value_string) - 1, "%" PRIdPTR, actual);
    const char *actual_value_constraint_name = "Expected [%s] to [%s]";
    const char *expected_value_name =  "[%s]\n";
    const char *expected_value_actual_value =
    		"\t\tactual value:\t[\"%s\"]\n"
            "\t\texpected value:\t[\"%s\"]\n";
    const char *at_offset = "\t\tat offset:\t[%d]\n";
    const char *actual_value = "\t\tactual value:\t[%" PRIdPTR "]\n";
    const char *expected_value_message = "\t\texpected value:\t[%" PRIdPTR "]\n";
    size_t message_size = strlen(actual_value_constraint_name) +
    		strlen(expected_value_name) +
    		strlen(expected_value_actual_value) +
    		strlen(at_offset) +
    		strlen(actual_value) +
    		strlen(expected_value_message) +
    		strlen(constraint->expected_value_name) +
    		strlen(constraint->name) +
    		strlen(actual_string) +
    		512; // just in case

    if (values_are_strings_in(constraint)) {
    	message_size += strlen((char *)constraint->expected_value);
    	if (actual != (intptr_t)NULL) {
    		message_size += strlen((char *)actual);
    	}
    }

    char *message = (char *)malloc(message_size);

    snprintf(message, message_size - 1,
    		actual_value_constraint_name,
    		actual_string,
            constraint->name);

    if (no_expected_value_in(constraint)) {
        strcat(message, "\n");
        return message;
    } else
        strcat(message, " ");

    snprintf(message + strlen(message), message_size - strlen(message) - 1,
            expected_value_name,
            constraint->expected_value_name);

    if (actual_value_not_necessary_for(constraint, actual_string, actual_value_string)) {
        /* when the actual string and the actual value are the same, don't print both of them */
        /* also, don't print "0" for false and "1" for true */
        /* also, don't print expected/actual for contents constraints since that is useless */
        return message;
    }

    /* for string constraints, print out the strings encountered and not their pointer values */
    if (values_are_strings_in(constraint)) {
        snprintf(message + strlen(message), message_size - strlen(message) - 1,
        			expected_value_actual_value,
                    (const char *)actual,
                    (const char *)constraint->expected_value);
        return message;
    }

    if(is_content_comparing(constraint)) {
        int difference_index = find_index_of_difference((char *)constraint->expected_value, (char *)actual, constraint->size_of_expected_value);
        snprintf(message + strlen(message), message_size - strlen(message) - 1,
                at_offset,
                difference_index);
        return message;
    }

    snprintf(message + strlen(message), message_size - strlen(message) - 1,
         actual_value,
         actual);

    if(strstr(constraint->name, "not ") == NULL) {
    snprintf(message + strlen(message), message_size - strlen(message) - 1,
        expected_value_message,
        constraint->expected_value);
    }

    return message;
}

#ifdef __cplusplus
}
#endif
