<?php
// $Id$

/*#############################################################################
#
#    pyro_db_gaskets.inc
#    
#    Code which implements funcitonality for pyrotag gaskets
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
#############################################################################*/

include_once('./'. drupal_get_path('module', 'pyro_db') .'/pyro_db.jobs.inc');

//////////////////////////////////////////////////////////////////////////////////////////////////
// Body
//////////////////////////////////////////////////////////////////////////////////////////////////

function _pyro_db_gasket_body($isSave, $nid) {
    if($isSave)
    {
        // called when the node is being saved or updated...
        // overwrite the body of the node so that we own it!
         $prog_body = "<"."?php _pyro_db_gasket_body(FALSE, $nid); ?".">";
         db_query("UPDATE {node_revisions} nr SET nr.body='".$prog_body."', nr.teaser='', nr.format='3' WHERE nid='".$nid."'");
    }
    else
    {
        // generate a warning if there are any smaple nids with no MID
        _pyro_db_gasket_genrate_unset_warning($nid);
        
        // select all the jobs assigned to this gasket...
        echo "<br><h1>GASKET INFORMATION</h1><hr>";
        $result = db_query("SELECT gas.field_gas_desc_value, gas.field_gas_num_samps_value, gas.field_gas_max_samples_value FROM {content_type_pdb_gasket} gas WHERE gas.nid ='$nid'");
        $nun_dun = true;
        $header = array('Type', 'Count');
        $rows = array();
        if($row = db_fetch_object($result)) 
        {
            $rows[] = array( array('data' => 'MAX', 'width' => '35%') , $row->field_gas_max_samples_value);
            $rows[] = array( array('data' => 'USED', 'width' => '35%') , $row->field_gas_num_samps_value);
        }
        
        echo "<h2>Description:</h2>";
        echo "$row->field_gas_desc_value<br>";
        
        echo theme_table($header, $rows);
        
        // get all the jobs which belong to this gasket
        $result = db_query("SELECT n.title AS node_title, n.nid AS nid FROM {node} n LEFT JOIN {content_type_pdb_client_job} cj_plus_node ON n.vid = cj_plus_node.vid WHERE cj_plus_node.field_cj_gasket_nid ='$nid'");
        $nun_dun = true;
        $header = array('Job', 'Samples');
        $rows = array();
        
        $ninny6_well_plate = array('A1' => 'EMPTY', 'B1' => 'EMPTY', 'C1' => 'EMPTY', 'D1' => 'EMPTY', 'E1' => 'EMPTY', 'F1' => 'EMPTY', 'G1' => 'EMPTY', 'H1' => 'EMPTY',
                              'A2' => 'EMPTY', 'B2' => 'EMPTY', 'C2' => 'EMPTY', 'D2' => 'EMPTY', 'E2' => 'EMPTY', 'F2' => 'EMPTY', 'G2' => 'EMPTY', 'H2' => 'EMPTY',
                              'A3' => 'EMPTY', 'B3' => 'EMPTY', 'C3' => 'EMPTY', 'D3' => 'EMPTY', 'E3' => 'EMPTY', 'F3' => 'EMPTY', 'G3' => 'EMPTY', 'H3' => 'EMPTY',
                              'A4' => 'EMPTY', 'B4' => 'EMPTY', 'C4' => 'EMPTY', 'D4' => 'EMPTY', 'E4' => 'EMPTY', 'F4' => 'EMPTY', 'G4' => 'EMPTY', 'H4' => 'EMPTY',
                              'A5' => 'EMPTY', 'B5' => 'EMPTY', 'C5' => 'EMPTY', 'D5' => 'EMPTY', 'E5' => 'EMPTY', 'F5' => 'EMPTY', 'G5' => 'EMPTY', 'H5' => 'EMPTY',
                              'A6' => 'EMPTY', 'B6' => 'EMPTY', 'C6' => 'EMPTY', 'D6' => 'EMPTY', 'E6' => 'EMPTY', 'F6' => 'EMPTY', 'G6' => 'EMPTY', 'H6' => 'EMPTY',
                              'A7' => 'EMPTY', 'B7' => 'EMPTY', 'C7' => 'EMPTY', 'D7' => 'EMPTY', 'E7' => 'EMPTY', 'F7' => 'EMPTY', 'G7' => 'EMPTY', 'H7' => 'EMPTY',
                              'A8' => 'EMPTY', 'B8' => 'EMPTY', 'C8' => 'EMPTY', 'D8' => 'EMPTY', 'E8' => 'EMPTY', 'F8' => 'EMPTY', 'G8' => 'EMPTY', 'H8' => 'EMPTY',
                              'A9' => 'EMPTY', 'B9' => 'EMPTY', 'C9' => 'EMPTY', 'D9' => 'EMPTY', 'E9' => 'EMPTY', 'F9' => 'EMPTY', 'G9' => 'EMPTY', 'H9' => 'EMPTY',
                              'A10' => 'EMPTY', 'B10' => 'EMPTY', 'C10' => 'EMPTY', 'D10' => 'EMPTY', 'E10' => 'EMPTY', 'F10' => 'EMPTY', 'G10' => 'EMPTY', 'H10' => 'EMPTY',
                              'A11' => 'EMPTY', 'B11' => 'EMPTY', 'C11' => 'EMPTY', 'D11' => 'EMPTY', 'E11' => 'EMPTY', 'F11' => 'EMPTY', 'G11' => 'EMPTY', 'H11' => 'EMPTY',
                              'A12' => 'EMPTY', 'B12' => 'EMPTY', 'C12' => 'EMPTY', 'D12' => 'EMPTY', 'E12' => 'EMPTY', 'F12' => 'EMPTY', 'G12' => 'EMPTY', 'H12' => 'EMPTY',);
        
        while ($row = db_fetch_object($result)) 
        {
            $num_samples = 0;
            $samples = _pyro_db_job_get_all_samples($row->nid);
            $samp_str = "";
            foreach ($samples as $current_sample)
            {
                $sample_link = _pyro_db_gasket_get_link($current_sample);
                $sample_well = _pyro_db_sample_get_current_wl($current_sample);
                $sample_UID = _pyro_db_sample_get_UID($current_sample);
                $ninny6_well_plate[$sample_well] = _pyro_db_sample_get_well_text($current_sample);
                $tmp_sample_node = node_load($current_sample);
                $samp_str .= "<a href=\"?q=$sample_link\">".$tmp_sample_node->title."</a>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;".$sample_well."&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;".$sample_UID."<br>";
                $num_samples++;
            }
            
            $job_link = _pyro_db_gasket_get_link($row->nid);
            $job_string = "<strong>Title:</strong>&nbsp;&nbsp;&nbsp;<a href=\"?q=$job_link\">".$row->node_title."</a><br>";

            $job_string .= "<strong># Samples:</strong>&nbsp;&nbsp;&nbsp;$num_samples";
            
            $rows[] = array( array('data' => $job_string, 'width' => '35%') , $samp_str);
            $nun_dun = false;
        }
        
        
        if($nun_dun) 
        {
            echo "No jobs assigned...<br>";
        }
        else
        {
        
            # make a pretty 96 well view
            echo "<br><h1>WELL LAYOUT</h1><hr>";
            $well_view = "<table border=\"solid black 1px\" cellpadding=\"6\" cellspacing=\"6\">".
                                "<tr><td></td><td><b>1</b></td><td><b>2</b></td><td><b>3</b></td><td><b>4</b></td><td><b>5</b></td><td><b>6</b></td><td><b>7</b></td><td><b>8</b></td><td><b>9</b></td><td><b>10</b></td><td><b>11</b></td><td><b>12</b></td></tr>".
                                "<tr><td><b>A</b></td><td>".$ninny6_well_plate['A1']."</td><td>".$ninny6_well_plate['A2']."</td><td>".$ninny6_well_plate['A3']."</td><td>".$ninny6_well_plate['A4']."</td><td>".$ninny6_well_plate['A5']."</td><td>".$ninny6_well_plate['A6']."</td><td>".$ninny6_well_plate['A7']."</td><td>".$ninny6_well_plate['A8']."</td><td>".$ninny6_well_plate['A9']."</td><td>".$ninny6_well_plate['A10']."</td><td>".$ninny6_well_plate['A11']."</td><td>".$ninny6_well_plate['A12']."</td></tr>".
                                "<tr><td><b>B</b></td><td>".$ninny6_well_plate['B1']."</td><td>".$ninny6_well_plate['B2']."</td><td>".$ninny6_well_plate['B3']."</td><td>".$ninny6_well_plate['B4']."</td><td>".$ninny6_well_plate['B5']."</td><td>".$ninny6_well_plate['B6']."</td><td>".$ninny6_well_plate['B7']."</td><td>".$ninny6_well_plate['B8']."</td><td>".$ninny6_well_plate['B9']."</td><td>".$ninny6_well_plate['B10']."</td><td>".$ninny6_well_plate['B11']."</td><td>".$ninny6_well_plate['B12']."</td></tr>".
                                "<tr><td><b>C</b></td><td>".$ninny6_well_plate['C1']."</td><td>".$ninny6_well_plate['C2']."</td><td>".$ninny6_well_plate['C3']."</td><td>".$ninny6_well_plate['C4']."</td><td>".$ninny6_well_plate['C5']."</td><td>".$ninny6_well_plate['C6']."</td><td>".$ninny6_well_plate['C7']."</td><td>".$ninny6_well_plate['C8']."</td><td>".$ninny6_well_plate['C9']."</td><td>".$ninny6_well_plate['C10']."</td><td>".$ninny6_well_plate['C11']."</td><td>".$ninny6_well_plate['C12']."</td></tr>".
                                "<tr><td><b>D</b></td><td>".$ninny6_well_plate['D1']."</td><td>".$ninny6_well_plate['D2']."</td><td>".$ninny6_well_plate['D3']."</td><td>".$ninny6_well_plate['D4']."</td><td>".$ninny6_well_plate['D5']."</td><td>".$ninny6_well_plate['D6']."</td><td>".$ninny6_well_plate['D7']."</td><td>".$ninny6_well_plate['D8']."</td><td>".$ninny6_well_plate['D9']."</td><td>".$ninny6_well_plate['D10']."</td><td>".$ninny6_well_plate['D11']."</td><td>".$ninny6_well_plate['D12']."</td></tr>".
                                "<tr><td><b>E</b></td><td>".$ninny6_well_plate['E1']."</td><td>".$ninny6_well_plate['E2']."</td><td>".$ninny6_well_plate['E3']."</td><td>".$ninny6_well_plate['E4']."</td><td>".$ninny6_well_plate['E5']."</td><td>".$ninny6_well_plate['E6']."</td><td>".$ninny6_well_plate['E7']."</td><td>".$ninny6_well_plate['E8']."</td><td>".$ninny6_well_plate['E9']."</td><td>".$ninny6_well_plate['E10']."</td><td>".$ninny6_well_plate['E11']."</td><td>".$ninny6_well_plate['E12']."</td></tr>".
                                "<tr><td><b>F</b></td><td>".$ninny6_well_plate['F1']."</td><td>".$ninny6_well_plate['F2']."</td><td>".$ninny6_well_plate['F3']."</td><td>".$ninny6_well_plate['F4']."</td><td>".$ninny6_well_plate['F5']."</td><td>".$ninny6_well_plate['F6']."</td><td>".$ninny6_well_plate['F7']."</td><td>".$ninny6_well_plate['F8']."</td><td>".$ninny6_well_plate['F9']."</td><td>".$ninny6_well_plate['F10']."</td><td>".$ninny6_well_plate['F11']."</td><td>".$ninny6_well_plate['F12']."</td></tr>".
                                "<tr><td><b>G</b></td><td>".$ninny6_well_plate['G1']."</td><td>".$ninny6_well_plate['G2']."</td><td>".$ninny6_well_plate['G3']."</td><td>".$ninny6_well_plate['G4']."</td><td>".$ninny6_well_plate['G5']."</td><td>".$ninny6_well_plate['G6']."</td><td>".$ninny6_well_plate['G7']."</td><td>".$ninny6_well_plate['G8']."</td><td>".$ninny6_well_plate['G9']."</td><td>".$ninny6_well_plate['G10']."</td><td>".$ninny6_well_plate['G11']."</td><td>".$ninny6_well_plate['G12']."</td></tr>".
                                "<tr><td><b>H</b></td><td>".$ninny6_well_plate['H1']."</td><td>".$ninny6_well_plate['H2']."</td><td>".$ninny6_well_plate['H3']."</td><td>".$ninny6_well_plate['H4']."</td><td>".$ninny6_well_plate['H5']."</td><td>".$ninny6_well_plate['H6']."</td><td>".$ninny6_well_plate['H7']."</td><td>".$ninny6_well_plate['H8']."</td><td>".$ninny6_well_plate['H9']."</td><td>".$ninny6_well_plate['H10']."</td><td>".$ninny6_well_plate['H11']."</td><td>".$ninny6_well_plate['H12']."</td></tr></table>";
            echo $well_view;
            
            # make a print version
            $sing_line_table = "bpob";
            echo "
                <SCRIPT TYPE='text/javascript'>
                <!--
                    function printVersion()
                    {           
                        var w = window.open(\"Surprise\", \"#\");
                        var d = w.document.open();
                        d.write('<!DOCTYPE html><html><head><link type=\"text/css\" rel=\"stylesheet\" media=\"print\" href=\"/sites/all/themes/ace/css/print.css?E\" /></head>');
                        d.write('<body>$well_view');
                        d.write('<h1><span style=\"font-size: 24px;\">Notes:</span><h1>');
                        d.write('</body></html>');
                        d.close();
                    }
                //-->
                </SCRIPT>";
            echo "<a href=\"#\" onClick=\"return printVersion()\">Printer friendly version</a>";
            echo "<br><h1>ASSIGNED JOBS</h1><hr>";
            echo theme_table($header, $rows);
        }
    }
}

function _pyro_db_gasket_get_link($nid) {
    //-----
    // return the url of the given nid
    //
    $link_query = db_query("SELECT complete_menu.link_path AS path FROM {nodehierarchy_menu_links} nml LEFT JOIN {menu_links} complete_menu ON nml.mlid = complete_menu.mlid WHERE nml.nid = '$nid'");
    while ($link_row = db_fetch_object($link_query)) 
    {
        return $link_row->path;
    }
    return '';
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Edit form
//////////////////////////////////////////////////////////////////////////////////////////////////

function _pyro_db_gasket_add_form(&$form, $form_state, $form_id) {
    // Use this for removing the jobs
    $nid = $form['nid']['#value'];
    if($nid == 0) { return; }
    $used = 0;
    // max will fail to be stored in the db when the form first loads. So we need to bootstrap it here...
    $max = $form['field_gas_max_samples']['0']['#default_value']['value'];
    $result = db_query("SELECT gas.field_gas_num_samps_value,  gas.field_gas_max_samples_value FROM {content_type_pdb_gasket} gas WHERE gas.nid ='$nid'");
    if($row = db_fetch_object($result)) 
    {
        $max = $row->field_gas_max_samples_value;
        $used = $row->field_gas_num_samps_value;
    }
    
    // get all the jobs which belong to no gasket
    $result = db_query("SELECT cj_plus_node.field_cj_samples_arrived_value AS arrived, cj_plus_node.field_cj_uid_value AS JUID, n.created AS created, n.uid AS user, n.title AS node_title, n.nid AS nid FROM {node} n INNER JOIN {content_type_pdb_client_job} cj_plus_node ON n.vid = cj_plus_node.vid WHERE cj_plus_node.field_cj_gasket_nid ='0' OR cj_plus_node.field_cj_gasket_nid is NULL");
    $first_run = true;
    $found_some_jobs = false;
    while ($row = db_fetch_object($result)) 
    {
        if(true == $first_run)
        {
            $form['pdb_non_assigned_jobs'] = array(
            '#title' => 'Check these boxes to add jobs to the gasket',
            '#type' => 'fieldset',
            '#collapsible' => TRUE,
            '#collapsed' => FALSE, 
            '#weight' => 0, 
            '#tree' => TRUE, 
            '#description' => t("There is a limit of $max samples available for each gasket. So far $used places have been used. This leaves ".($max - $used)." available"),
            ); 
            $first_run = false;
            $found_some_jobs = true;
        }
        
        // get the number of samples that this guy has
        $job_nid = $row->nid;
        $this_user=user_load($row->user);
        $username=$this_user->name;
        $format = 'D - d M Y';
        $time = date($format, $row->created);
        $num_samples = _pyro_db_job_get_num_of_samples($job_nid);
        
        $simple_path = "node/".$job_nid;
        $simple_path =drupal_get_path_alias($simple_path);       
        
        $title = "$username :: $time :: $row->node_title (job UID: $row->JUID) --  $num_samples sample(s) -- ".l("LINK",t($simple_path));
        $form['pdb_non_assigned_jobs'][$row->node_title] = array(
            '#type' => 'checkbox',
            '#id' => t($job_nid),
            '#title' => $title,
            '#default_value' => 0,
            '#num_samples' => $num_samples,
        );
        if($row->arrived == "Awaiting arrival")
        {
            $form['pdb_non_assigned_jobs'][$row->node_title]['#title'] = "<span style=\"color: #CCC;\">$title</span>";
            $form['pdb_non_assigned_jobs'][$row->node_title]['#disabled'] = TRUE;
        }
    }   
    if(false == $found_some_jobs)
    {
        $form['pdb_non_assigned_jobs'] = array(
            '#title' => 'There are currently no jobs which can be added to this gasket',
            '#type' => 'fieldset',
            '#weight' => 1, 
        );  
    }
}

function _pyro_db_gasket_remove_form(&$form, $form_state, $form_id) {
    // Use this for removing the jobs
    $nid = $form['nid']['#value'];
    if($nid == 0) { return; }
    
    // get all the jobs which belong to this gasket
    $result = db_query("SELECT n.title AS node_title, n.nid AS nid FROM {node} n LEFT JOIN {content_type_pdb_client_job} cj_plus_node ON n.vid = cj_plus_node.vid WHERE cj_plus_node.field_cj_gasket_nid ='$nid'");
    $first_run = true;
    $found_some_jobs = false;
    while ($row = db_fetch_object($result)) 
    {
        if(true == $first_run)
        {
            $form['pdb_assigned_jobs'] = array(
                '#title' => 'Check these boxes to remove these jobs from the gasket. All MIDs for all samples will be reset!',
                '#type' => 'fieldset',
                '#collapsible' => TRUE,
                '#collapsed' => TRUE, 
                '#weight' => 1, 
                '#tree' => TRUE, 
            );
            $found_some_jobs= true;
            $first_run = false;
        }
        $nid = $row->nid;
        $num_samples = count(_pyro_db_job_get_all_samples($nid));
        $form['pdb_assigned_jobs'][$row->node_title] = array(
            '#type' => 'checkbox',
            '#title' => "$row->node_title  --  $num_samples sample(s)",
            '#id' => t($nid),
            '#default_value' => 0,
            '#num_samples' => $num_samples,
        );
        $found_some_jobs= true;
    }
    
    if(false == $found_some_jobs)
    {
        $form['pdb_assigned_jobs'] = array(
            '#title' => 'This gasket currently has no jobs that can be removed',
            '#type' => 'fieldset',
            '#weight' => 1, 
        );
    }
}

function _pyro_db_gasket_form_submit($form, &$form_state) {
    //-----
    // Do the heavy lifting
    //
    $nid = $form_state['values']['nid'];
    
    // get the total amount used so far
    $used = 0;
    // max will fail to be stored in the db when the form first loads. So we need to bootstrap it here...
    $max = $form['field_gas_max_samples']['0']['#default_value']['value'];
    $result = db_query("SELECT gas.field_gas_num_samps_value,  gas.field_gas_max_samples_value FROM {content_type_pdb_gasket} gas WHERE gas.nid ='$nid'");
    if($row = db_fetch_object($result)) 
    {
        $max = $row->field_gas_max_samples_value;
        $used = $row->field_gas_num_samps_value;
    }

    if(is_array($form_state['values']['pdb_assigned_jobs']))
    {
        foreach ( $form_state['values']['pdb_assigned_jobs'] as $key => $value)
        {
            if($value == 1)
            {
                $rem_job_nid = $form['pdb_assigned_jobs'][$key]['#id'];
                if("edit-pdb-assigned-jobs-submit" != $rem_job_nid) {
                    _pyro_db_gasket_do_remove($nid, $rem_job_nid);
                }
            }
        }
        $result = db_query("SELECT gas.field_gas_num_samps_value,  gas.field_gas_max_samples_value FROM {content_type_pdb_gasket} gas WHERE gas.nid ='$nid'");
        if($row = db_fetch_object($result)) 
        {
            $form_state['values']['field_gas_num_samps'][0]['value'] = $row->field_gas_num_samps_value;
        }
    }
    if(is_array($form_state['values']['pdb_non_assigned_jobs']))
    {
        $num_added = 0;
        foreach ( $form_state['values']['pdb_non_assigned_jobs'] as $key => $value)
        {
            if($value == 1)
            {
                $new_job_nid = $form['pdb_non_assigned_jobs'][$key]['#id'];
                if("edit-pdb-non-assigned-jobs-submit" != $new_job_nid) {
                    $num_added += _pyro_db_gasket_do_add($max, $used, $nid, $new_job_nid);
                }
            }
        }
        if(0 != $num_added)
        {
            $form_state['values']['field_gas_num_samps'][0]['value'] = ($used + $num_added);
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// MID lists
//////////////////////////////////////////////////////////////////////////////////////////////////

function _pyro_db_gasket_available_MIDS($mid_length, $nid) {
    //-----
    // return all the available mids...
    // 
    // set length to -1 to get all lengths
    //
    $query = "";
    $ret_array = array();
    
    // get all the mids currently assigned to this gasket
    $used_array = _pyro_db_gasket_used_MIDS($nid);
    // get all mids of the specified length
    if($mid_length == -1)
    {
        $query = "SELECT ctpm.field_454_mid_sequence_value, ctpm.field_454_mid_id_value FROM {content_type_pyrotag_mid} ctpm WHERE ctpm.field_454_mid_state_value ='active' ORDER BY ctpm.field_454_mid_id_value ASC";
    }
    else
    {
        $query = "SELECT ctpm.field_454_mid_sequence_value, ctpm.field_454_mid_id_value FROM {content_type_pyrotag_mid} ctpm WHERE ctpm.field_454_mid_state_value ='active' AND ctpm.field_454_mid_length_value = '$mid_length' ORDER BY ctpm.field_454_mid_id_value ASC";
    }
    $all_result = db_query($query);
    while ($valid_mid = db_fetch_object($all_result)) 
    {
        if(! in_array($valid_mid->field_454_mid_sequence_value, $used_array))
        {
            $ret_array[] = $valid_mid->field_454_mid_sequence_value;
        }
    }
    return $ret_array;
}

function _pyro_db_gasket_used_MIDS($nid) {
    //-----
    // get all the mids currently assigned to this gasket
    //
    $used_array = array();
    $assigned_result = db_query("SELECT pgm.mid FROM {pdb_gasket_mids} pgm WHERE pgm.gasket_id = '$nid'");
    while ($used_mid = db_fetch_object($assigned_result)) 
    {
        $used_array[] = $used_mid->mid;
    }
    return $used_array;
}

function _pyro_db_gasket_set_mids($job_nid, $mid_length, $nid) {
    //-----
    // assign MIDs for all the jobs in the given array...
    //
    $sample_list = _pyro_db_job_get_all_samples($job_nid);
    
    // check that there are enough free MIDs
    // to give one to each sample...
    $free_mids = _pyro_db_gasket_available_MIDS($mid_length, $nid);
    if(count($sample_list) > count($free_mids))
    {
        $tmp_job_node = node_load($job_nid);
        $len_str = "";
        if(-1 != $mid_length)
        {
            $len_str = " of length".$mid_length;
        }
        drupal_set_message($tmp_job_node->title." has ".count($sample_list)." sample(s) but only ".count($free_mids)." mid(s)".$len_str." are free", 'error', true );
        return false;
    }
    $current_mid_id_ref = 0;
    foreach ($sample_list as $current_sample)
    {
        _pyro_db_sample_set_mid($nid, $free_mids[$current_mid_id_ref], $current_sample);
        $current_mid_id_ref++;
    }
    return true;
}

function _pyro_db_gasket_unset_mids($job_nid, $nid) {
    //-----
    // un-assign MIDs for all samples for the current job
    //
    $sample_list = _pyro_db_job_get_all_samples($job_nid);
    foreach ($sample_list as $current_sample)
    {
        _pyro_db_sample_unset_mid($current_sample);
    }
}

function _pyro_db_is_unset_mids($nid) {
    //-----
    // Compile a list of any unset mids
    //
    $ret_array = array();
    $result = db_query("SELECT pgm.sample_id from {pdb_gasket_mids} pgm WHERE gasket_id = '$nid' AND mid = ''");
    while ($row = db_fetch_object($result)) {
        $ret_array[$row->sample_id] = 1;
    }
    return $ret_array;
}

function _pyro_db_gasket_genrate_unset_warning($nid) {
    //-----
    // if the mid_array is non-empty, generate a drupal-esque 
    // warning
    //
    $mid_array = _pyro_db_is_unset_mids($nid);
    $gas_node = node_load($nid);
    foreach($mid_array as $samp_nid => $flag)
    {
        $samp_node = node_load($samp_nid);
        drupal_set_message("Gasket: ".$gas_node->title." has sample: ".$samp_node->title." with unset MID!", 'warning', TRUE);
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Add remove jobs
//////////////////////////////////////////////////////////////////////////////////////////////////

function _pyro_db_gasket_do_add($max, $used, $gs_nid, $job_nid) {
    //-----
    // add a job to a gasket and do all that entails
    //
    $tmp_job_node = node_load($job_nid);
    $job_title = $tmp_job_node->title;
    $tmp_gs_node = node_load($gs_nid);
    $num_samples = count(_pyro_db_job_get_all_samples($job_nid));
    if($num_samples > ($max - $used)) {
        drupal_set_message("Adding $job_title to gasket may make gasket too dense. There is a limit of $max samples per gasket and adding this job would mean that this gasket has ".($used + $num_samples)." samples", 'warning', TRUE);
    }
    
    if(_pyro_db_gasket_set_mids($job_nid, -1, $gs_nid))
    {
        // add the job to the gasket
        drupal_set_message("Added: $job_title to ".$tmp_gs_node->title, 'status', TRUE);
        db_query("UPDATE {content_type_pdb_client_job} ctpcj set ctpcj.field_cj_gasket_nid ='$gs_nid' WHERE ctpcj.nid = '$job_nid'");
        
        // lock it!
        _pyro_db_job_lock_job($tmp_job_node, true);
        
        // update the sample count
        //$num_samples += $used;
        return $num_samples;
    }
    else
    {
        drupal_set_message("Could NOT add: ".$tmp_job_node->title." to ".$tmp_gs_node->title, 'error', TRUE);
    }
    return 0;
}

function _pyro_db_gasket_do_remove($gs_nid, $job_nid) {
    //-----
    // remove a job from a gasket and do all that entails
    // This function will call a function which is responsible for freeing
    // the MIDS and for decrementing the count. The function is: _pyro_db_sample_unset_mid
    // Do not change this behaviour. This is becuase and admin delete of a sample must fix the gasker used count!
    $tmp_job_node = node_load($job_nid);
    $tmp_gs_node = node_load($gs_nid);
    
    drupal_set_message("Removed: ".$tmp_job_node->title." from ".$tmp_gs_node->title, 'status', TRUE);
    db_query("UPDATE {content_type_pdb_client_job} ctpcj set ctpcj.field_cj_gasket_nid = '0' WHERE ctpcj.nid = '$job_nid'");
    
    // unset the MIDs 
    _pyro_db_gasket_unset_mids($job_nid, $gs_nid);
    
    // unlock the jobs and sample for editing
    _pyro_db_job_lock_job($tmp_job_node, false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Workflows
//////////////////////////////////////////////////////////////////////////////////////////////////

function _pyro_db_gasket_is_locked($nid) {
    //-----
    // Is this gasket locked?
    // state 8 is open, state 9 is locked!
    //
    $result = db_query("SELECT wn.sid FROM {workflow_node} wn WHERE nid = '$nid'");
    while ($row = db_fetch_object($result)) {
        if($row->sid == 8) { return false; }
        elseif ($row->sid == 9) { return true; }
    }
    return false;
}

function _pyro_db_gasket_can_lock($nid) {
    //-----
    // Check and see if there are any unset MIDS
    // prevent locking if so!
    //
    $result = db_query("SELECT pgm.sample_id from {pdb_gasket_mids} pgm WHERE gasket_id = '$nid' AND mid = ''");
    while ($row = db_fetch_object($result)) {
        return false;
    }
    return true;
}
