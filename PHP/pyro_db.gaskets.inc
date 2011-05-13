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
        // select all the jobs assigned to this gasket...
        echo "<br><h1>SAMPLE COUNT</h1><hr>";
        $result = db_query("SELECT gas.field_gas_num_samps_value,  gas.field_gas_max_samples_value FROM {content_type_pdb_gasket} gas WHERE gas.nid ='$nid'");
        $nun_dun = true;
        $header = array('Type', 'Count');
        $rows = array();
        if($row = db_fetch_object($result)) 
        {
            $rows[] = array( array('data' => 'MAX', 'width' => '35%') , $row->field_gas_max_samples_value);
            $rows[] = array( array('data' => 'USED', 'width' => '35%') , $row->field_gas_num_samps_value);
        }
        echo theme_table($header, $rows);
        
        // select all the jobs assigned to this gasket...
        echo "<br><h1>ASSIGNED JOBS</h1><hr>";
        // get all the jobs which belong to this gasket
        $result = db_query("SELECT n.title AS node_title, n.nid AS nid FROM {node} n LEFT JOIN {content_type_pdb_client_job} cj_plus_node ON n.vid = cj_plus_node.vid WHERE cj_plus_node.field_cj_gasket_nid ='$nid'");
        $nun_dun = true;
        $header = array('Job', 'Samples');
        $rows = array();
        
        while ($row = db_fetch_object($result)) 
        {
            $samples = _pyro_db_job_get_all_samples($row->nid);
            $samp_str = "";
            foreach ($samples as $current_sample)
            {
                $tmp_sample_node = node_load($current_sample);
                $samp_str .= $tmp_sample_node->title."<br>";
            }
            $rows[] = array( array('data' => $row->node_title, 'width' => '35%') , $samp_str);
            $nun_dun = false;
        }
        if($nun_dun) 
        {
            echo "No jobs assigned...<br>";
        }
        else
        {
            echo theme_table($header, $rows);
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Edit form
//////////////////////////////////////////////////////////////////////////////////////////////////

function _pyro_db_gasket_add_form(&$form, $form_state, $form_id) {
    // Use this for removing the jobs
    $nid = $form['nid']['#value'];
    $used = 0;
    // max will fail to be stored in the db when the form first loads. So we need to bootstrap it here...
    $max = $form['field_gas_max_samples']['0']['#default_value']['value'];
    $result = db_query("SELECT gas.field_gas_num_samps_value,  gas.field_gas_max_samples_value FROM {content_type_pdb_gasket} gas WHERE gas.nid ='$nid'");
    if($row = db_fetch_object($result)) 
    {
        $max = $row->field_gas_max_samples_value;
        $used = $row->field_gas_num_samps_value;
    }
    
    $form['pdb_non_assigned_jobs'] = array(
        '#title' => 'Check these boxes to add jobs to the gasket',
        '#type' => 'fieldset',
        '#collapsible' => TRUE,
        '#collapsed' => FALSE, 
        '#weight' => 0, 
        '#tree' => TRUE, 
        '#description' => t("There is a limit of $max samples available for each gasket. So far $used places have been used. This leaves ".($max - $used)." available"),
    );

    // get all the jobs which belong to no gasket
    $result = db_query("SELECT n.title AS node_title, n.nid AS nid FROM {node} n INNER JOIN {content_type_pdb_client_job} cj_plus_node ON n.vid = cj_plus_node.vid WHERE cj_plus_node.field_cj_gasket_nid ='0' OR cj_plus_node.field_cj_gasket_nid is NULL");
    while ($row = db_fetch_object($result)) 
    {
        // get the number of samples that this guy has
        $nid = $row->nid;
        $num_samples = count(_pyro_db_job_get_all_samples($nid));
        $form['pdb_non_assigned_jobs'][$row->node_title] = array(
            '#type' => 'checkbox',
            '#id' => t($nid),
            '#title' => "$row->node_title  --  $num_samples sample(s)",
            '#default_value' => 0,
            '#num_samples' => $num_samples,
        );
    }
}

function _pyro_db_gasket_remove_form(&$form, $form_state, $form_id) {
    // Use this for removing the jobs
    $nid = $form['nid']['#value'];
    
    $form['pdb_assigned_jobs'] = array(
        '#title' => 'Check these boxes to remove these jobs from the gasket. All MIDs for all samples will be reset!',
        '#type' => 'fieldset',
        '#collapsible' => TRUE,
        '#collapsed' => TRUE, 
        '#weight' => 1, 
        '#tree' => TRUE, 
    );

    // get all the jobs which belong to this gasket
    $result = db_query("SELECT n.title AS node_title, n.nid AS nid FROM {node} n LEFT JOIN {content_type_pdb_client_job} cj_plus_node ON n.vid = cj_plus_node.vid WHERE cj_plus_node.field_cj_gasket_nid ='$nid'");
    while ($row = db_fetch_object($result)) 
    {
        $nid = $row->nid;
        $num_samples = count(_pyro_db_job_get_all_samples($nid));
        $form['pdb_assigned_jobs'][$row->node_title] = array(
            '#type' => 'checkbox',
            '#title' => "$row->node_title  --  $num_samples sample(s)",
            '#id' => t($nid),
            '#default_value' => 0,
            '#num_samples' => $num_samples,
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
            if($value)
            {
                $rem_job_nid = $form['pdb_assigned_jobs'][$key]['#id'];
                if("edit-pdb-assigned-jobs-submit" != $rem_job_nid) {
                    $form_state['values']['field_gas_num_samps'][0]['value'] =- _pyro_db_gasket_do_remove($nid, $rem_job_nid);
                }
            }
        }
    }
    if(is_array($form_state['values']['pdb_non_assigned_jobs']))
    {
        foreach ( $form_state['values']['pdb_non_assigned_jobs'] as $key => $value)
        {
            if($value)
            {
                $new_job_nid = $form['pdb_non_assigned_jobs'][$key]['#id'];
                if("edit-pdb-non-assigned-jobs-submit" != $new_job_nid) {
                    $form_state['values']['field_gas_num_samps'][0]['value'] = _pyro_db_gasket_do_add($max, $used, $nid, $new_job_nid);
                }
            }
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
        $query = "SELECT ctpm.field_454_mid_sequence_value FROM {content_type_pyrotag_mid} ctpm WHERE ctpm.field_454_mid_state_value ='active'";
    }
    else
    {
        $query = "SELECT ctpm.field_454_mid_sequence_value FROM {content_type_pyrotag_mid} ctpm WHERE ctpm.field_454_mid_state_value ='active' AND ctpm.field_454_mid_length_value = '$mid_length'";
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
    $num_freed = 0;
    $sample_list = _pyro_db_job_get_all_samples($job_nid);
    foreach ($sample_list as $current_sample)
    {
        _pyro_db_sample_unset_mid($current_sample);
        $num_freed++;
    }
    return $num_freed;
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
    if($num_samples > ($max - $used)) { drupal_set_message("Adding $job_title to gasket may make gasket too dense. There is a limit of $max samples per gasket and adding this job would mean that this gasket has ".($used + $num_samples)." samples", 'warning', TRUE);}
    
    if(_pyro_db_gasket_set_mids($job_nid, -1, $gs_nid))
    {
        // add the job to the gasket
        drupal_set_message("Added: $job_title to ".$tmp_gs_node->title, 'status', TRUE);
        db_query("UPDATE {content_type_pdb_client_job} ctpcj set ctpcj.field_cj_gasket_nid ='$gs_nid' WHERE ctpcj.nid = '$job_nid'");
        
        // lock it!
        _pyro_db_job_lock_job($tmp_job_node, true);
        
        // update the sample count
        $num_samples += $used;
        return $num_samples;
    }
    else
    {
        drupal_set_message("Could NOT add: ".$tmp_job_node->title." to ".$tmp_gs_node->title, 'error', TRUE);
    }
    return -1;
}

function _pyro_db_gasket_do_remove($gs_nid, $job_nid) {
    //-----
    // remove a job from a gasket and do all that entails
    // This function will call a function which is responsible for freeing
    // the MIDS and for decrementing the count. The function is: _pyro_db_sample_unset_mid
    // Do not change this behaviour. This is becuase and admin delete of a sample must fix the gasker used count!
    // this function returns the number freed
    $tmp_job_node = node_load($job_nid);
    $tmp_gs_node = node_load($gs_nid);
    
    drupal_set_message("Removed: ".$tmp_job_node->title." from ".$tmp_gs_node->title, 'status', TRUE);
    db_query("UPDATE {content_type_pdb_client_job} ctpcj set ctpcj.field_cj_gasket_nid = '0' WHERE ctpcj.nid = '$job_nid'");
    
    // unset the MIDs 
    $num_freed = _pyro_db_gasket_unset_mids($job_nid, $gs_nid);
    
    // unlock the jobs and sample for editing
    _pyro_db_job_lock_job($tmp_job_node, false);
    
    return $num_freed;
}
