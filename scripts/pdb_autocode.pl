#!/usr/bin/perl
###############################################################################
#
#    pdb_autocode.pl
#    
#    Autocodes A LOT of functionality for the PyroDB
#
#    Copyright (C) 2011 Michael Imelfort
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
###############################################################################

#pragmas
use strict;
use warnings;

#core Perl modules
use Getopt::Long;

#CPAN modules

#locally-written modules

BEGIN {
    select(STDERR);
    $| = 1;
    select(STDOUT);
    $| = 1;
}

# get input params and print copyright
my $options = checkParams();
if(!exists $options->{'silent'}) { printAtStart(); }

######################################################################
# CODE HERE
######################################################################

# globals
# Each line looks like this:
# human_skin  dermatology_disord  dermatology disorder    history of dermatology disorders; can include multiple disorders    X   disorder name   {text}  m
# we can split on tab and then get all the info we need:
#
# Field: -> description
# 0         metadata_type
# 1         db field name
# 2         fancy name
# 3         description
# 4         required?
# 5         expected input
# 6         format
# 7         number expected

my $global_title = '';
my @global_fields = ();            # db names
my @global_fancies = ();           # display names
my @global_descriptions = ();      # descriptions for the input form
my @global_requireds = ();         # if this field is required
my @global_expecteds = ();         # if this field is required
my @global_formats = ();           # strict format expected
my @global_nums = ();              # number of entries we're expecting
my $global_name = '';              # edit form group name

open my $in_fh, "<", $options->{'infile'};

# load the input file
parseInput();

# make the SQL
auto_sql_line();

# make short db name...
my $global_short_name = '';
my @short_name_bits = split /_/, $global_title;
foreach my $bit (@short_name_bits)
{
    $global_short_name .= substr($bit, 0 , 1);
}

# make al lthe codes!
open my $out_fh, ">", $global_title.".inc";

print $out_fh "<?php\n";
print $out_fh "  // \n";
print $out_fh "  // WARNING!\n";
print $out_fh "  // EVERYTHING IN THIS FILE HAS BEEN AUTOMATICALLY CODED.\n";
print $out_fh "  // EDIT BY HAND AT YOUR OWN PERIL!\n";
print $out_fh "  // \n\n";

auto_table_addremove ();
auto_types ();
auto_insert ();
auto_update();
auto_get();
auto_view ();
auto_fancies();
auto_delete();
auto_clone();
auto_form_component();
auto_form_validate();
auto_form_submit();

close $out_fh;

######################################################################
# CUSTOM SUBS
######################################################################
sub parseInput {
    # parse from the input file
    my $nl_count = 0;
    while(<$in_fh>)
    {
        chomp $_;

        my @line_bits = split /\t/, $_;
        $global_name = uc($line_bits[0]);
        $global_title = "pdb_metadata_".$line_bits[0];
        
        # fix the formats field
        my $inp_format = $line_bits[6];
        $inp_format =~ s/\} \{/\};\{/;
        $inp_format =~ s/; /;/;
        # first we see if we have any '[' chars in the format
        my $bracket_count = $inp_format =~ tr/[//;
        if(0 != $bracket_count)
        {
            # count the number of '|' chars in the string
            my $bar_count = ($inp_format =~ tr/|//);
            my @splitz = split /\|/, $inp_format;
            $inp_format = "";
            my $counter = 0;
            foreach my $bit (@splitz)
            {
                $inp_format .= $bit;
                if($counter < $bar_count) { $inp_format .= " = $counter, "; }
                $counter++;
            }
            $counter--;
            my $rep_str = " = $counter";
            $inp_format =~ s/]/$rep_str]/;
        }
        
        # fix the fancies, description and expecteds fields
        $line_bits[2] = ucfirst($line_bits[2]);
        $line_bits[3] = ucfirst($line_bits[3]);
        $line_bits[5] = ucfirst($line_bits[5]);
        
        push @global_fields, $line_bits[1];
        push @global_fancies, $line_bits[2];
        push @global_descriptions, $line_bits[3];
        if($line_bits[4] eq "M")
        {
            push @global_requireds, "true";
        }
        else
        {
            push @global_requireds, "false";
        }
        push @global_expecteds, $line_bits[5];
        push @global_formats, $inp_format;
        push @global_nums, $line_bits[7];
    }    
}

sub auto_sql_line {
    # output the sql entry
    print "DROP TABLE IF EXISTS `$global_title`;\n";
    print "CREATE TABLE `$global_title` (`nid` int(10) unsigned NOT NULL";
    my $counter = 0;
    foreach my $key (@global_fields)
    {
        print ", `$key` varchar(200)";
        $counter++;
    }
    print ", PRIMARY KEY (`nid`) ) ENGINE=MyISAM DEFAULT CHARSET=utf8;\n";

}
######################
# ADDREMOVE
######################
sub auto_table_addremove {
    print $out_fh "function _pyro_db_".$global_title."_addremove(\$nid, \$add)\n{\n";
    print $out_fh "  //-----\n";
    print $out_fh "  // AUTOMATICALLY CODED. EDIT AT YOUR OWN PERIL\n";
    print $out_fh "  // \n";
    print $out_fh "  // Register this node for this metadata in the pdb_metadata_tables table\n";
    print $out_fh "  // Note: If this node does not already have an entry it will be added automagically\n";
    print $out_fh "  // \n";
    print $out_fh "    if(\$add) {\n";
    print $out_fh "        \$result = db_query(\"SELECT * FROM {pdb_metadata_tables}  where nid='\".\$nid.\"'\");\n";
    print $out_fh "        while (\$row = db_fetch_object(\$result)) {\n";
    print $out_fh "            db_query(\"UPDATE {pdb_metadata_tables} SET ".$global_title."='1' where nid='\".\$nid.\"'\");\n";
    print $out_fh "            return;\n";
    print $out_fh "        }\n";
    print $out_fh "        db_query(\"INSERT INTO {pdb_metadata_tables} (nid, ".$global_title.") VALUES ('\".\$nid.\"', '1')\");\n";
    print $out_fh "    } else {\n";
    print $out_fh "        db_query(\"UPDATE {pdb_metadata_tables} SET ".$global_title."='0' where nid='\".\$nid.\"'\");\n";
    print $out_fh "    }\n";
    print $out_fh "}\n\n";
}
    
######################
# INSERT
######################
sub auto_insert {
    print $out_fh "function _pyro_db_".$global_title."_insert(\$values)\n{\n";
    print $out_fh "  //-----\n";
    print $out_fh "  // AUTOMATICALLY CODED. EDIT AT YOUR OWN PERIL\n";
    print $out_fh "  // Insert values into table: $global_title\n";
    print $out_fh "  // This function takes an associative array of key=>value pairs.\n";
    print $out_fh "  // The key is the name of the table column and the value is the value to insert\n";
    print $out_fh "  // It's up to you (or a validation script) to make sure that\n";
    print $out_fh "  // all of the keys and values make sense!\n";
    print $out_fh "  // \n";
    print $out_fh "  // This function expects the input array to contain:\n";
    print $out_fh "  // \n";
    print $out_fh "  // nid => int(10)\n";
    my $counter = 0;
    foreach my $key (@global_fields)
    {
        print $out_fh "  // $key => varchar(200)\n";
        $counter++;
    }
    print $out_fh "  // \n";

    # make the insert statement
    print $out_fh "    \$ins_query = \"INSERT INTO {$global_title} (\";\n";
    print $out_fh "    \$db_types = _pyro_db_".$global_title."_types();\n";
    print $out_fh "    \$db_vars = array();\n";
    # make a PHP foreach loop
    print $out_fh "    \$first = true;\n";
    print $out_fh "    foreach (\$values as \$key => \$value) {\n";
    print $out_fh "        if(!\$first) { \$ins_query .= \", \"; }\n";
    print $out_fh "        \$first = false;\n";
    print $out_fh "        \$ins_query .= \$key;\n";
    print $out_fh "    }\n";
    print $out_fh "    \$ins_query .= \") VALUES (\";\n";

    print $out_fh "    \$first = true;\n";
    print $out_fh "    foreach (\$values as \$key => \$value) {\n";
    print $out_fh "        if(!\$first) { \$ins_query .= \", \"; }\n";
    print $out_fh "        \$first = false;\n";
    print $out_fh "        if(is_null(\$value))\n";
    print $out_fh "        { \$ins_query .= \"NULL\"; }\n";
    print $out_fh "        else\n";
    print $out_fh "        { \$ins_query .= \"'\".\$db_types[\$key].\"'\"; \$db_vars[] = \$value; }\n";
    print $out_fh "    }\n";
    print $out_fh "    \$ins_query .= \")\";\n";
    print $out_fh "\n    db_query(\$ins_query, \$db_vars);\n";
    print $out_fh "    _pyro_db_".$global_title."_addremove(\$values['nid'], TRUE);\n";
    print $out_fh "}\n\n";
}

sub auto_types {
    print $out_fh "function _pyro_db_".$global_title."_types()\n";
    print $out_fh "{\n";
    print $out_fh "     return array(\n        'nid' => '%d',\n";
    my $counter = 0;
    foreach my $key (@global_fields)
    {
        print $out_fh "        '$key' => '\%s',\n";
        $counter++;
    }
    print $out_fh "    );\n";
    print $out_fh "}\n\n";
}

######################
# UPDATE
######################
sub auto_update {
    print $out_fh "function _pyro_db_".$global_title."_update(\$values)\n{\n";
    print $out_fh "  //-----\n";
    print $out_fh "  // AUTOMATICALLY CODED. EDIT AT YOUR OWN PERIL\n";
    print $out_fh "  // Update values in table: $global_title\n";
    print $out_fh "  // This function takes an associative array of key=>value pairs.\n";
    print $out_fh "  // The key is the name of the table column and the value is the value to insert\n";
    print $out_fh "  // It's up to you (or a validation script) to make sure that\n";
    print $out_fh "  // all of the keys and values make sense!\n";
    print $out_fh "  // \n";
    print $out_fh "  // The first entry in the array MUST be 'nid' => ID\=n";
    print $out_fh "  // \n";
    print $out_fh "  // The input array can contain (but doesn't need to):\n";
    print $out_fh "  // \n";
    my $counter = 0;
    foreach my $key (@global_fields)
    {
        print $out_fh "  // $key => varchar(200)\n";
        $counter++;
    }
    print $out_fh "  // \n";

    # make the insert statement
    print $out_fh "    \$ins_query = \"UPDATE {$global_title} SET \";\n";
    print $out_fh "    \$db_types = _pyro_db_".$global_title."_types();\n";
    print $out_fh "    \$db_vars = array();\n";
    # make a PHP foreach loop
    print $out_fh "    \$first = true;\n";
    print $out_fh "    foreach (\$values as \$key => \$value) {\n";
    print $out_fh "        if(\$key != 'nid') {\n";
    print $out_fh "            if(!\$first) { \$ins_query .= \", \"; }\n";
    print $out_fh "            \$first = false;\n";
    print $out_fh "            \$ins_query .= \$key.\"='\".\$db_types[\$key].\"'\";\n";
    print $out_fh "            \$db_vars[] = \$value;\n";
    print $out_fh "        }\n";
    print $out_fh "    }\n";
    print $out_fh "    \$ins_query .= \" where nid='\".\$values['nid'].\"'\";\n";
    print $out_fh "\n    db_query(\$ins_query, \$db_vars);\n";
    print $out_fh "}\n\n";
}

######################
# GET
######################
sub auto_get {
    print $out_fh "function _pyro_db_".$global_title."_get(\$nid)\n{\n";
    print $out_fh "  //-----\n";
    print $out_fh "  // AUTOMATICALLY CODED. EDIT AT YOUR OWN PERIL\n";
    print $out_fh "  // Get values in table: $global_title for the given nid\n";
    print $out_fh "  // The return array will contain:\n";
    print $out_fh "  // \n";
    print $out_fh "    \$ins_query = \"SELECT * FROM {$global_title} where nid='\".\$nid.\"'\";\n";
    print $out_fh "\n    \$result = db_query(\$ins_query);\n";
    print $out_fh "    \$ret_array = array();\n";
    print $out_fh "    if (\$tmp_array = db_fetch_object(\$result)) {\n";
    print $out_fh "        foreach (\$tmp_array as \$key => \$value)\n";
    print $out_fh "        {\n";
    print $out_fh "            if('nid' != \$key)\n";
    print $out_fh "            {\n";
    print $out_fh "                \$ret_array[\$key] = \$value;\n";
    print $out_fh "            }\n";
    print $out_fh "        }\n";
    print $out_fh "    }\n";
    print $out_fh "    return \$ret_array;\n";
    print $out_fh "}\n\n";
}

######################
# VIEW
######################
sub auto_view  {
    print $out_fh "function _pyro_db_".$global_title."_view(\$nid)\n{\n";
    print $out_fh "  //-----\n";
    print $out_fh "  // AUTOMATICALLY CODED. EDIT AT YOUR OWN PERIL\n";
    print $out_fh "  // View values in table: $global_title for the given nid\n";
    print $out_fh "  // Echoes HTML code\n";
    print $out_fh "  // \n";
    print $out_fh "    \$get_array = _pyro_db_".$global_title."_get(\$nid);\n";
    print $out_fh "    \$header = array('Field', 'Value');\n";
    print $out_fh "    \$fancy_names = _pyro_db_".$global_title."_get_fancy();\n";
    print $out_fh "    \$rows = array();\n";
    print $out_fh "    \$any_made = false;\n";
    
    print $out_fh "    foreach(\$get_array as \$key => \$value) {\n";
    print $out_fh "        if(NULL != \$value) {\n";
    print $out_fh "            \$rows[] = array( array('data' => '<B>'.\$fancy_names[\$key].'</B>', 'width' => '35%'), \$value);\n";
    print $out_fh "            \$any_made = true;\n";
    print $out_fh "        }\n";
    print $out_fh "    }\n";
    print $out_fh "    if(\$any_made) {\n";
    print $out_fh "        echo \"<br><h2>METADATA :: ".$global_name."</h2>\";\n";
    print $out_fh "        echo theme_table(\$header, \$rows);\n";
    print $out_fh "    }\n";
    print $out_fh "}\n\n";
}

sub auto_fancies {
    print $out_fh "function _pyro_db_".$global_title."_get_fancy()\n{\n";
    print $out_fh "  //-----\n";
    print $out_fh "  // AUTOMATICALLY CODED. EDIT AT YOUR OWN PERIL\n";
    print $out_fh "  // \n";
    print $out_fh "  // Get the fancy names for the table columns\n";
    print $out_fh "  //\n";
    
    print $out_fh "     return array(\n";
    my $counter = 0;
    foreach my $key (@global_fields)
    {
        print $out_fh "        '$key' => '$global_fancies[$counter]',\n";
        $counter++;
    }
    print $out_fh "    );\n";
    print $out_fh "}\n\n";
}

######################
# DELETE
######################
sub auto_delete {
    print $out_fh "function _pyro_db_".$global_title."_delete(\$nid)\n{\n";
    print $out_fh "  //-----\n";
    print $out_fh "  // AUTOMATICALLY CODED. EDIT AT YOUR OWN PERIL\n";
    print $out_fh "  // Delete this sample\n";
    print $out_fh "  // \n";
    # drop the existing row if any
    print $out_fh "    \$ins_query = \"DELETE FROM {$global_title} where nid='\".\$nid.\"'\";\n";
    print $out_fh "\n    db_query(\$ins_query);\n";
    print $out_fh "    _pyro_db_".$global_title."_addremove(\$nid, FALSE);\n";
    print $out_fh "}\n\n";
}

######################
# CLONE
######################
sub auto_clone {
    print $out_fh "function _pyro_db_".$global_title."_clone(\$nid_src, \$nid_dest)\n{\n";
    print $out_fh "  //-----\n";
    print $out_fh "  // AUTOMATICALLY CODED. EDIT AT YOUR OWN PERIL\n";
    print $out_fh "  // Clone this table for another sample\n";
    print $out_fh "  // \n";
    # drop the existing row if any
    print $out_fh "    _pyro_db_".$global_title."_delete(\$nid_dest);\n";
    # get the row of the source
    print $out_fh "    \$get_array = _pyro_db_".$global_title."_get(\$nid_src);\n";
    # and insert
    print $out_fh "    _pyro_db_".$global_title."_addremove(\$nid_dest, true);\n";
    print $out_fh "    \$get_array['nid'] = \$nid_dest;\n";
    print $out_fh "    _pyro_db_".$global_title."_insert(\$get_array);\n";
    print $out_fh "}\n\n";
}

######################
# FORM COMPONENT
######################
sub auto_form_component {
    print $out_fh "function _pyro_db_".$global_title."_form_alter(&\$form, \$form_state, &\$rep_array)\n";
    print $out_fh "{\n";
    print $out_fh "    \$form['".$global_title."'] = array(\n";
    print $out_fh "        '#type' => 'fieldset',\n";
    print $out_fh "        '#title' => t('METADATA :: ".$global_name."'),\n";
    print $out_fh "        '#collapsible' => TRUE,\n";
    print $out_fh "        '#collapsed' => TRUE, \n";
    print $out_fh "        '#weight' => 100, \n";
    print $out_fh "        '#tree' => TRUE, \n";
    print $out_fh "    );\n";
    
    # need some way to pre-populate with values
    
    print $out_fh "    \$nid = \$form['nid']['#value'];\n";
    print $out_fh "    \$get_array = _pyro_db_".$global_title."_get(\$nid);\n\n";
    
    my $counter = 0;
    foreach my $key (@global_fields)
    {
        # set the description
        my $description = "<strong>Description:&nbsp;&nbsp;&nbsp;</strong>$global_descriptions[$counter]<br>";
        $description .= "<strong>Expected value:&nbsp;&nbsp;&nbsp;</strong>$global_expecteds[$counter]<br>";
        $description .= "<strong>Format:&nbsp;&nbsp;&nbsp;</strong>$global_formats[$counter]";
        if($global_formats[$counter] =~ /timestamp/)
        {
            $description .= "&nbsp;&nbsp;&nbsp; [YYYY]-[MM]-[DD]T[hh]:[mm]:[ss](+-[hh]:[mm] if has offset from UTC)&nbsp;&nbsp;EX: 2007-04-05T22:30:15+10";
        }
        elsif($global_formats[$counter] =~ /interval/)
        {
            $description .= "&nbsp;&nbsp;&nbsp; [YYYY]-[MM]-[DD]T[hh]:[mm]:[ss]+-[hh]:[mm]/[YYYY]-[MM]-[DD]T[hh]:[mm]:[ss]+-[hh]:[mm]<br>(+-[hh]:[mm] if has offset from UTC)&nbsp;&nbsp;EX: 2007-03-01T13:00:00+10/2008-05-11T15:30:00+10";
        }
        elsif($global_formats[$counter] =~ /duration/ || $global_formats[$counter] =~ /period/)
        {
            $description .= "<br>P[n]Y[n]M[n]DT[n]H[n]M[n]S > P3Y6M4DT12H30M5S<br>";
            $description .= "o P is the duration designator (historically called \"period\") placed at the start of the duration representation.<br>";
            $description .= "o Y is the year designator that follows the value for the number of years.<br>";
            $description .= "o M is the month designator that follows the value for the number of months.<br>";
            $description .= "o W is the week designator that follows the value for the number of weeks.<br>";
            $description .= "o D is the day designator that follows the value for the number of days.<br>";
            $description .= "o T is the time designator that precedes the time components of the representation.<br>";
            $description .= "o H is the hour designator that follows the value for the number of hours.<br>";
            $description .= "o M is the minute designator that follows the value for the number of minutes.<br>";
            $description .= "o S is the second designator that follows the value for the number of seconds.";
        }
        
        $description .= "<br>";
        
        # set the size
        my $size = 80;
        my $maxsize = 200;
        my $type = 'textfield';
        if($global_nums[$counter] eq "m") {
            $type = 'textarea';
            $description .= "Multiple values are OK, one entry per line";
        }
        else
        {
            $description .= "Please enter only one value";
        }
        
        print $out_fh auto_form_elements($key, $type, $global_fancies[$counter], '', '',  $description, $size, $maxsize, $global_requireds[$counter], "\$rep_array");
        $counter++;
    }
    print $out_fh "}\n\n";
    
}
    
######################
# FORM VALIDATE
######################
sub auto_form_validate {
    print $out_fh "function _pyro_db_".$global_title."_form_validate(&\$form, \$form_state)\n";
    print $out_fh "{\n";
    print $out_fh "}\n\n";
}

######################
# FORM SUBMIT
######################
sub auto_form_submit {
    print $out_fh "function _pyro_db_".$global_title."_form_submit(&\$form, \$form_state)\n";
    print $out_fh "{\n";
    print $out_fh "    \$nid = \$form['nid']['#value'];\n";
    print $out_fh "    \$get_array = _pyro_db_".$global_title."_get(\$nid);\n\n";
    print $out_fh "    \$meta_updated = false;\n";
    print $out_fh "    \$submitta = array('nid' => \$nid,);\n";
    print $out_fh "    foreach (\$get_array as \$key => \$value)\n";
    print $out_fh "    {\n";
    print $out_fh "        if(\$form_state['values']['".$global_title."'][\$key] != \$value)\n";
    print $out_fh "        {\n";
    print $out_fh "            \$submitta[\$key] = \$form_state['values']['".$global_title."'][\$key];\n";
    print $out_fh "            \$meta_updated = true;\n";
    print $out_fh "        }\n";
    print $out_fh "    }\n";
    print $out_fh "    if(\$meta_updated) { _pyro_db_".$global_title."_update(\$submitta); }\n";
    print $out_fh "}\n\n";
}

######################
# FORM ELEMENTS
######################
sub auto_form_elements {
    #
    # Creates form elements
    #
    my ($id, $type, $title, $default_value, $value, $description, $size, $maxlength, $required, $visible) = @_;
    my $ret_string = "    if(NULL != \$get_array['$id']) { \$default_value = \$get_array['$id']; } else { \$default_value = ''; }\n";
    $ret_string .= "    \$access = true;\n";
    $ret_string .= "    if(\$rep_array[\"$title\"] == 1) { \$access = false; } else { \$rep_array[\"$title\"] = 1; }\n";
    $ret_string .= "    \$form['".$global_title."']['$id'] = array(\n";
    $ret_string .= "        '#type' => '$type',\n";
    $ret_string .= "        '#title' => t('$title'),\n";
    $ret_string .= "        '#default_value' => \$default_value,\n";
    $ret_string .= "        '#description' => '$description',\n";
    $ret_string .= "        '#size' => $size,\n";
    $ret_string .= "        '#maxlength' => $maxlength,\n";
    $ret_string .= "        '#required' => $required,\n";
    $ret_string .= "        '#access' => \$access,\n";
    $ret_string .= "    );\n\n";
    return $ret_string;
}

#drupal_set_message('Made '.$num_clones.' copie(s) of this node ', 'status', TRUE);
#form_set_error('new_field_name', 'AHH SOMETHING WRONG!');

######################################################################
# TEMPLATE SUBS
######################################################################
sub checkParams {
    my @standard_options = ( "help|h+", "infile|i:s", "silent|s+" );
    my %options;

    # Add any other command line options, and the code to handle them
    # 
    GetOptions( \%options, @standard_options );

    # if no arguments supplied print the usage and exit
    #
    exec("pod2usage $0") if (0 == (keys (%options) ));

    # If the -help option is set, print the usage and exit
    #
    exec("pod2usage $0") if $options{'help'};

    exec("pod2usage $0") if (!exists $options{'infile'});

    return \%options;
}

sub printAtStart {
print<<"EOF";
---------------------------------------------------------------- 
 $0
 Copyright (C) 2011 Michael Imelfort
    
 This program comes with ABSOLUTELY NO WARRANTY;
 This is free software, and you are welcome to redistribute it
 under certain conditions: See the source for more details.
---------------------------------------------------------------- 
EOF
}

__DATA__

=head1 NAME

    pdb_autocode.pl

=head1 COPYRIGHT

   copyright (C) 2011 Michael Imelfort

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

=head1 DESCRIPTION

   Insert detailed description here

=head1 SYNOPSIS

    pdb_autocode.pl  -infile|i STRING [-silent|s] [-help|h]

      -infile -i                   The file to parse
      -silent -s		   Suppress all NON-NECESSARY screen noise
      [-help -h]                   Displays basic usage information
         
=cut

