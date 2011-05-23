<?php
// $Id$

/*#############################################################################
#
#    pyro_db.inc
#    
#    Code which implements sitewide funcitonality
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

//////////////////////////////////////////////////////////////////////////////////////////////////
// Displaying jobs
//////////////////////////////////////////////////////////////////////////////////////////////////

function _pyro_db_display_myjobs() {
    echo "<div id=\"pdb_wrapper_div\" style=\"visibility:visible; position:relative;\" >\n";
    echo drupal_get_form('_pyro_db_myjobs_form');
    echo "</div><!-- pdb_wrapper_div -->\n";
}

function _pyro_db_myjobs_form($form_state) {
    $form = array();
    $form['#submit'][] = '_pyro_db_myjobs_submit';
    $form['#validate'][] = '_pyro_db_myjobs_validate';
    if(isset($form_state['storage']['values']))
    {
        $submitted_data = $form_state['storage']['values'];
        if(isset($submitted_data['filter_by_gasket']))
        {
            echo "<div id=\"filtered\" style=\"visibility: visible; position: absolute; top: 40px;\">\n";
            echo _pyro_db_myJobsPage($submitted_data['filter_by_gasket']);
            echo "</div><!-- filtered -->\n";
            echo "<script type=\"text/javascript\">_pdb_toggleVisibility('pdb_unfiltered');</script>";
            
        }
        $form['submit'] = array('#type' => 'submit', '#value' => t('Reset filter'));
    }
    else
    {
        // is this guy an admin?
        global $user;
        $is_admin = false;
        $result = db_query('SELECT r.name FROM {users_roles} ur LEFT JOIN {role} r ON r.rid=ur.rid WHERE ur.uid=%d', $user->uid);
        $css_string = "";
        while ($row = db_fetch_object($result)) 
        {
            if("PyroDB_admin" == $row->name)
            {
                $is_admin = true;
                $css_string = "position: absolute; top: 130px;";
                break;
            }
        }
        echo "<div id=\"pdb_unfiltered\" style=\"visibility: visible; $css_string\">\n";
        echo _pyro_db_myJobsPage('');
        echo "</div><!-- pdb_unfiltered -->\n";
        if($is_admin)
        {
            $form['filter_by_gasket'] = array(
                '#type' => 'select',
                '#options' => _pyro_db_get_gaskets(),
                '#title' => t('Filter by Gasket'),
                '#size' => 1,
                '#description' => t('Choose the gasket to display jobs for'),
            );
            $form['submit'] = array('#type' => 'submit', '#value' => t('Filter...'));
        }
    }

    $form['nid'] = array(
        '#type' => 'hidden',
        '#value' => $nid,
    );
    return $form;
}

function _pyro_db_myjobs_validate($form, &$form_state){}


function _pyro_db_myjobs_submit($form, &$form_state) {
    //-----
    // when yo submit bra!
    //
    if(isset($form_state['storage']['values']))
    {
        unset($form_state['storage']);
    }
    else
    {
        $form_state['storage']['values'] = $form_state['values'];
    }
}

function _pyro_db_myJobsPage($gasket)
{
    //-----
    // the "MyJobs" page
    //
    global $user;
    // is this guy an admin?
    $is_admin = false;
    $result = db_query('SELECT r.name FROM {users_roles} ur LEFT JOIN {role} r ON r.rid=ur.rid WHERE ur.uid=%d', $user->uid);
    while ($row = db_fetch_object($result)) 
    {
        if("PyroDB_admin" == $row->name)
        {
            $is_admin = true;
            break;
        }
    }

    $query = "SELECT ctpcj.field_cj_uid_value AS UNID, n.nid FROM {node} n INNER JOIN {users} u ON n.uid = u.uid INNER JOIN {content_type_pdb_client_job} ctpcj ON n.nid = ctpcj.nid WHERE (n.type in ('pdb_client_job'))";
    if(!$is_admin) { $query .= " AND (u.uid = ".$user->uid.")"; }
    if('' != $gasket) { $query .= " AND (ctpcj.field_cj_gasket_nid = '$gasket')"; }
    $query .= " ORDER BY n.changed DESC";

    return _pyro_db_myJobsPage_render($query, $is_admin);
}

function _pyro_db_myJobsPage_render($query, $isAdmin)
{
    //-----
    // the "MyJobs" page
    //
    global $user;
    $result = db_query($query);
    $big_ret_string = "";
    
    while ($row = db_fetch_object($result)) 
    {
        // get nice pretty display info
        
        // first work out the job info
        $current_job_id = $row->nid;
        $path = drupal_get_path_alias("node/$current_job_id");
        $tmp_job_node = node_load($current_job_id);
        $job_header = "Job: <a href=\"?q=$path\">".$tmp_job_node->title."</a>&nbsp;&nbsp;&nbsp;&nbsp; UID: ".$row->UNID."&nbsp;&nbsp;&nbsp;&nbsp;";
        
        if($isAdmin)
        {
            // now the user info
            $tmp_user = user_load(array('uid' => $tmp_job_node->uid));
            $group_nid = _pyro_db_get_client_group_nid($tmp_user->uid);
            $group_node = node_load($group_nid);
            
            $usr_str = "Created by: <a href=\"?q=user/".$tmp_user->uid."\">".$tmp_user->name."</a>&nbsp;&nbsp;&nbsp;&nbsp;From group: <a href=\"?q=node/$group_nid\">".$group_node->title."</a>&nbsp;&nbsp;&nbsp;&nbsp;";
            $gas_info = _pyro_db_job_get_gasket($current_job_id);
            $gas_str = "";
            if(0 != count($gas_info))
            {
                $gas_path = drupal_get_path_alias("node/".$gas_info['nid']);
                $gas_str = "Assigned to gasket: <a href=\"?q=$gas_path\">".$gas_info['title']."</a>";
            }
            $job_header .= $usr_str.$gas_str;
            
        }
        $header = array($job_header);
        $rows = array();
        $view = views_get_view('job_sample_table');
        $rows[] = array( $view->execute_display('default', array($current_job_id)) );

        $big_ret_string .= theme_table($header, $rows);
    }
    
    if("" == $big_ret_string)
    {
        $big_ret_string = "Nothing to show: you do not have any jobs...<br>";
    }
    return $big_ret_string;
}

function _pyro_db_get_gaskets()
{
    //-----
    // return nids for all gaskets
    //
    $ret_array = array();
    $query = "SELECT ctpg.nid as nid, n.title as title from {content_type_pdb_gasket} ctpg LEFT JOIN {node} n ON n.nid = ctpg.nid";
    $result = db_query($query);
    $counter = 0;
    while ($row = db_fetch_object($result)) 
    {
        $ret_array[$row->nid] = $row->title;
        $counter++;
    }
    return $ret_array;
}

function _pyro_db_drupe_alert($message) { drupal_set_message($message , 'status', true); }
