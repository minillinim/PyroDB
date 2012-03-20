<?php
// $Id$
/*#############################################################################
#
#    pyro_db_jobs.inc
#    
#    Code which implements funcitonality for pyrotag jobs
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

include_once('./'. drupal_get_path('module', 'pyro_db') .'/pyro_db.conf.inc');

//////////////////////////////////////////////////////////////////////////////////////////////////
// Active metafields
//////////////////////////////////////////////////////////////////////////////////////////////////

function _pyro_db_get_meta_fields() {
    //-----
    // update this array to add / remove a metadata field
    //
    return array(
        'AIR' => 'pdb_metadata_air',
        'HOST ASSOCIATED' => 'pdb_metadata_host_associated',
        'HUMAN ASSOCIATED' => 'pdb_metadata_human_associated',
        'HUMAN GUT' => 'pdb_metadata_human_gut',
        'HUMAN ORAL' => 'pdb_metadata_human_oral',
        'HUMAN SKIN' => 'pdb_metadata_human_skin',
        'HUMAN VAGINAL' => 'pdb_metadata_human_vaginal',
        'MAT BIOFILM' => 'pdb_metadata_mat_biofilm',
        'PLANT ASSOCIATED' => 'pdb_metadata_plant_associated',
        'SEDIMENT' => 'pdb_metadata_sediment',
        'SOIL' => 'pdb_metadata_soil',
        'WASTE WATER SLUDGE' => 'pdb_metadata_wastewater_sludge',
        'WATER' => 'pdb_metadata_water',
        'MISCELLANEOUS ENVIRONMENT' => 'pdb_metadata_miscellaneous',
    );
}

function _pyro_db_job_active_meta_fields($nid) {
    //-----
    // Work out which fields are active...
    //
    $ret_array = array();
    $meta_fields = _pyro_db_get_meta_fields();
    if(NULL == $nid)
    {
        // on the creation page
        foreach($meta_fields as $key => $value)
        {
            $ret_array[$value] = 0;
        }
    }
    else
    {
        // not on the creation page
        $meta_fields = array_flip($meta_fields);
        $query = "select * from {pdb_metadata_tables} pdb where nid='$nid'";
        $result = db_query($query);
        while ($row = db_fetch_object($result)) 
        {    
            foreach($row as $key => $value)
            {
                if(isset($meta_fields[$key]))
                {
                    $ret_array[$key] = $value;
                }
            }
        }
    }
    return $ret_array;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Body
//////////////////////////////////////////////////////////////////////////////////////////////////

function _pyro_db_job_body($isSave, $nid) {
    if($isSave)
    {
        // called when the node is being saved or updated...
        $prog_body = "<"."?php _pyro_db_job_body(FALSE, $nid); ?".">";
        db_query("UPDATE {node_revisions} nr SET nr.body='".$prog_body."', nr.teaser='', nr.format='3' WHERE nid='".$nid."'");
        db_query("UPDATE {content_type_pdb_client_job} ctpcj SET ctpcj.field_cj_gasket_nid='0' WHERE nid='".$nid."'");
    }
    else
    {
        print drupal_get_form('_pyro_db_job_clone_form', $nid);
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Edit form
//////////////////////////////////////////////////////////////////////////////////////////////////

function _pyro_db_job_form_alter(&$form, $form_state, $form_id) {
    //-----
    // alters the edit form for a job page...
    //
    $form['metadata_fields'] = array(
        '#title' => 'Select which metadata fields you would like to use<br>NOTE: Removing an existing type of metadata will permanently remove the information from ALL associated samples',
        '#type' => 'fieldset',
        '#collapsible' => TRUE,
        '#collapsed' => FALSE, 
        '#weight' => 100, 
        '#tree' => TRUE, 
    );
    
    $nid = $form['nid']['#value'];
    $meta_fields = _pyro_db_get_meta_fields();
    $active_fields = _pyro_db_job_active_meta_fields($nid);
    
    foreach($meta_fields as $header => $col_name) 
    {
        $form['metadata_fields'][$col_name] = array(
        '#type' => 'checkbox',
        '#title' => t($header),
        '#default_value' => $active_fields[$col_name],
        );
    }
    
    // if the node is unlocked then we can lock it!
    $result = db_query("SELECT wn.sid FROM {workflow_node} wn WHERE wn.nid = '$nid'");
    while ($row = db_fetch_object($result)) {
        if($row->sid == 2)
        {
            $form['lock_job'] = array(
                '#title' => 'Update this value ONLY when you have finsihed adding all samples to this job',
                '#type' => 'fieldset',
                '#collapsible' => TRUE,
                '#collapsed' => TRUE, 
                '#weight' => 102, 
                '#tree' => TRUE, 
            );
            
            $form['lock_job']['self_lock'] = array(
                '#title' => "I'm done!",
                '#description' => t('Check this box when you have finished adding ALL of your samples. After the job has been locked you will no longer be able to add or remove samples. You will be able edit the information. If you continue MIDs can be assigned to the samples and sequencing can commence. This can NOT be undone'),
                '#type' => 'checkbox',
                '#default_value' => 0,
            );
        }
    }
    
    $form['#validate'][] = '_pyro_db_job_validate';
    $form['#submit'][] = '_pyro_db_job_submit';
    $form['buttons']['submit']['#submit'][] = '_pyro_db_job_redirect_handler';
    
    // if the gasket is locked, do not allow deleting
    if(_pyro_db_job_gasket_is_locked($nid))
    {
        unset($form['buttons']['delete']);
    }
}

function _pyro_db_job_validate($form, &$form_state) {
    //-----
    // not needed but here just in case
    //
}
    
function _pyro_db_job_submit($form, &$form_state) {
    //-----
    // Update / add / remove metadata
    //
    // we need to work out if anything has changed!
    $nid = $form_state['values']['nid'];
    if(NULL != $nid)
    {
        // not in creation. node_hook will handle all the creation stuff
        $active_fields = _pyro_db_job_active_meta_fields($nid);
        $flipped_meta_fields = array_flip(_pyro_db_get_meta_fields());
        $sample_nids = _pyro_db_job_get_all_samples($nid);
        
        // are there any remaining active fields which 
        // contain the string human but are not the generic human associated?
        $hu_states = array();
        
        // is the user trying to remove the human associated field?
        $try_del_ha = false;
        
        // go through the form elems
        foreach ($form_state['values']['metadata_fields'] as $key => $value)
        {
            if($value == 1)
            {
                // this is an active field!
                // is this a human associated meta field?
                if(_pyro_db_sample_is_human_meta($key) && $key != "pdb_metadata_human_associated" )
                {
                    $hu_states[$key] = true;
                }
                    
                if($active_fields[$key] == 0)
                {
                    // added one
                    drupal_set_message("Added $flipped_meta_fields[$key] to ALL samples", 'status', true);
                    foreach($sample_nids as $s_nid)
                    {
                        // add the sample to the "pdb_tables" as well as all the actual metadata storing tables
                        $func = "_pyro_db_$key"."_addremove";
                        $func($s_nid, true);
                        
                        $passing_array = array('nid' => $s_nid);
                        $func = "_pyro_db_$key"."_insert";
                        $func($passing_array);
                        
                        // Fix the job node too
                        $func = "_pyro_db_$key"."_addremove";
                        $func($nid, true);
                    }
                }
            }
            elseif(($value == 0) && ($active_fields[$key] == 1))
            {
                // removed one
                // check to see if the user is trying to remove ha here...
                if($key == "pdb_metadata_human_associated")
                {
                    // yes!
                    $try_del_ha = true;
                }
                else
                {
                    // is this a human associated meta field?
                    if(_pyro_db_sample_is_human_meta($key))
                    {
                        $hu_states[$key] = false;
                    }
                    
                    drupal_set_message("Removed $flipped_meta_fields[$key] from ALL samples and deleted any entered data", 'status', true);
                    foreach($sample_nids as $s_nid)
                    {
                        // remove the sample to the "pdb_tables" as well as all the actual metadata storing tables
                        $func = "_pyro_db_$key"."_addremove";
                        $func($s_nid, false);
                        
                        $func = "_pyro_db_$key"."_delete";
                        $func($s_nid);
                        
                        // Fix the job node too
                        $func = "_pyro_db_$key"."_addremove";
                        $func($nid, false);
                    }
                }
            }
        }
        
        // reload the list of active fields
        $active_fields = _pyro_db_job_active_meta_fields($nid);
        
        // check if there are still any active human fields
        $active_hu_exists = false;
        {
            foreach($hu_states as $key => $value)
            {
                if($value) { $active_hu_exists = true; }
            }
        }
        
        // now see if we need to use this information
        $key = "pdb_metadata_human_associated";
        if($active_hu_exists)
        {
            if($try_del_ha)
            {
                // user is trying to delete ha while these still exist other fields
                // this is a no no
                drupal_set_message("Could not remove HUMAN ASSOCIATED METADATA while other human associated fields exist", 'warning', true);
            }
            
            if($active_fields[$key] == 0)
            {
                // user has included some human associated field but has not included  the generic one.
                drupal_set_message("Added $flipped_meta_fields[$key] to ALL samples", 'status', true);
                foreach($sample_nids as $s_nid)
                {
                    // add the sample to the "pdb_tables" as well as all the actual metadata storing tables
                    $func = "_pyro_db_$key"."_addremove";
                    $func($s_nid, true);
                    
                    $passing_array = array('nid' => $s_nid);
                    $func = "_pyro_db_$key"."_insert";
                    $func($passing_array);
                    
                    // Fix the job node too
                    $func = "_pyro_db_$key"."_addremove";
                    $func($nid, true);
                }
            }
        }
        else
        {
            // no human associated tables exist
            // if the user is trying to remove one, let them
            if($try_del_ha)
            {
                drupal_set_message("Removed $flipped_meta_fields[$key] from ALL samples and deleted any entered data", 'status', true);
                foreach($sample_nids as $s_nid)
                {
                    // remove the sample to the "pdb_tables" as well as all the actual metadata storing tables
                    $func = "_pyro_db_$key"."_addremove";
                    $func($s_nid, false);
                    
                    $func = "_pyro_db_$key"."_delete";
                    $func($s_nid);
                    
                    // Fix the job node too
                    $func = "_pyro_db_$key"."_addremove";
                    $func($nid, false);
                }
            }
        }

        // check to see if the user is done!
        if(isset($form_state['values']['lock_job']['self_lock']))
        {
            if($form_state['values']['lock_job']['self_lock'] == 1)
            {
                // user locked the job
                $tmp_job_node = node_load($nid);
                _pyro_db_job_lock_job($tmp_job_node, true);
                drupal_set_message("Job '$tmp_job_node->title' and all samples have been locked, admin will be notified and MIDs will be assigned soon", 'status', true);
            }
        }
    }
}

function _pyro_db_job_redirect_handler($form, &$form_state) {
    //-----
    // redirect after a user self-locks a node
    //
    $nid = $form_state['values']['nid'];
    if(NULL != $nid)
    {        
        if(isset($form_state['values']['lock_job']['self_lock']))
        {
            if($form_state['values']['lock_job']['self_lock'] == 1)
            {
                $result = db_query("SELECT nhml.mlid FROM {nodehierarchy_menu_links} nhml WHERE nhml.nid = '$nid'");
                while ($row = db_fetch_object($result)) {
                    $result3 = db_query("SELECT ml.link_title, ml.link_path FROM {menu_links} ml WHERE ml.mlid = '".$row->mlid."'");
                    while ($row3 = db_fetch_object($result3)) {
                        $result4 = db_query("SELECT ua.dst FROM {url_alias} ua WHERE ua.src = '".$row3->link_path."'");
                        while($row4 = db_fetch_object($result4)) {
                            $form_state['redirect'] = $row4->dst;
                            break;
                        }
                        break;
                    }
                    break;
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Workflows
//////////////////////////////////////////////////////////////////////////////////////////////////

function _pyro_db_job_lock_job($job_node, $lockIt) {
    //-----
    // lock or unlock the workflow
    //
    if($lockIt)
    {
        // do the job
        // State 3 is the locked state for a job
        workflow_execute_transition($job_node, 3);
        // do the samples
        $all_samples = _pyro_db_job_get_all_samples($job_node->nid);
        foreach ($all_samples as $this_sample)
        {
            $tmp_sample_node = node_load($this_sample);
            // State 6 is the locked state for a sample
            workflow_execute_transition($tmp_sample_node, 6);
        }
    }
    else
    {
        // do the job
        // State 2 is the open state for a job
        workflow_execute_transition($job_node, 2);
        // do the samples
        $all_samples = _pyro_db_job_get_all_samples($job_node->nid);
        foreach ($all_samples as $this_sample)
        {
            $tmp_sample_node = node_load($this_sample);
            // State 5 is the open state for a sample
            workflow_execute_transition($tmp_sample_node, 5);
        }
    }
}

function _pyro_db_job_gasket_is_locked($nid) {
    //-----
    // Is this job's gasket locked?
    //
    $result = db_query("SELECT ctpcj.field_cj_gasket_nid FROM {content_type_pdb_client_job} ctpcj WHERE ctpcj.nid = '$nid'"); 
    while ($row = db_fetch_object($result)) {
        $result2 = db_query("SELECT wn.sid FROM {workflow_node} wn WHERE nid = '$row->field_cj_gasket_nid'");
        while ($row2 = db_fetch_object($result2)) {
            if($row2->sid == 8) { return false; }
            elseif ($row2->sid == 9) { return true; }
        }
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Sample / Gasket management
//////////////////////////////////////////////////////////////////////////////////////////////////

function _pyro_db_job_first_child($nid) {
    $nh_menu_link = 0;
    $result = db_query("SELECT nhml.mlid FROM {nodehierarchy_menu_links} nhml WHERE nhml.nid = '$nid'");
    while ($row = db_fetch_object($result)) {
        $result2 = db_query("SELECT ml.link_path FROM {menu_links} ml WHERE ml.plid = '".$row->mlid."'");
        while ($row2 = db_fetch_object($result2)) {
            return $row2->link_path.'/edit';
        }    
    }
    return 0;
}
    
function _pyro_db_increment_job_sample_counter($nid) {
    $result = db_query("SELECT ctpcj.field_cj_samp_num_value FROM {content_type_pdb_client_job} ctpcj WHERE ctpcj.nid = '$nid'");
    $samp_num = 0;
    while ($row = db_fetch_object($result)) {
        $samp_num = $row->field_cj_samp_num_value + 1;
        db_query("UPDATE {content_type_pdb_client_job} ctpcj set ctpcj.field_cj_samp_num_value ='".$samp_num."' WHERE ctpcj.nid = '$nid'");
        break;
    }
    return $samp_num;
}

function _pyro_db_job_get_all_samples($nid) {
    //----
    // get all the samples for this job
    //
    $ret_array = array();
    $query = "SELECT DISTINCT(node.nid) AS nid_1,
     node.nid AS nid,
     nh_menu_links.weight AS nh_menu_links_weight 
     FROM {node} node 
     LEFT JOIN nodehierarchy_menu_links nodehierarchy_menu_links ON node.nid = nodehierarchy_menu_links.nid 
     LEFT JOIN menu_links nh_menu_links ON nodehierarchy_menu_links.mlid = nh_menu_links.mlid 
     LEFT JOIN nodehierarchy_menu_links nh_parent ON nh_menu_links.plid = nh_parent.mlid 
     WHERE (node.status <> 0) AND (nh_parent.nid = ".$nid.") 
     GROUP BY nid_1 
     ORDER BY nh_menu_links_weight ASC";
    $result = db_query($query);
    while($row = db_fetch_object($result)) 
    {
        $ret_array[] = $row->nid;
    }
    return $ret_array;
}

function _pyro_db_job_get_num_of_samples($nid) {
    //----
    // get all the samples for this job
    //
    $ret_array = array();
    $query = "SELECT COUNT(*)
     FROM {node} node 
     LEFT JOIN nodehierarchy_menu_links nodehierarchy_menu_links ON node.nid = nodehierarchy_menu_links.nid 
     LEFT JOIN menu_links nh_menu_links ON nodehierarchy_menu_links.mlid = nh_menu_links.mlid 
     LEFT JOIN nodehierarchy_menu_links nh_parent ON nh_menu_links.plid = nh_parent.mlid 
     WHERE (node.status <> 0) AND (nh_parent.nid = ".$nid.")";
    return db_result(db_query($query));
}

function _pyro_db_job_get_gasket($nid) {
    //-----
    // return the gasket for this job 
    //
    $ret_array = array();
    $query = "SELECT ctpcj.field_cj_gasket_nid AS gas_nid, node.title AS gas_title FROM {content_type_pdb_client_job} ctpcj INNER JOIN {node} node ON node.nid = ctpcj.field_cj_gasket_nid WHERE ctpcj.nid ='$nid'";
    $result = db_query($query);
    while($row = db_fetch_object($result)) 
    {
        $ret_array['nid'] = $row->gas_nid;
        $ret_array['title'] = $row->gas_title;
        return $ret_array;
    }
    return $ret_array;
}

function _pyro_db_job_get_UID($job_nid) {
    //-----
    // wrapper to get the sample UID
    // returns 'X!X!X' is not set.
    //
    $result = db_query("SELECT ctpcj.field_cj_uid_value FROM {content_type_pdb_client_job} ctpcj where nid='$job_nid'");
    while ($row = db_fetch_object($result)) 
    {
        if('' != $row->field_cj_uid_value)
        {
            return $row->field_cj_uid_value;
        }
    }
    return 'X!X!X';
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Job Cloning
//////////////////////////////////////////////////////////////////////////////////////////////////

function _pyro_db_job_clone_form($form_state, $nid) {
    $form = array();
    $form['#submit'][] = '_pyro_db_clone_job_submit';
    $form['#validate'][] = '_pyro_db_clone_job_validate';
    
    if(isset($form_state['storage']['values'])) {  
        $query = "SELECT DISTINCT(node.nid) AS nid,
            node.title AS node_title,
            node_data_field_cs_id.field_cs_id_value AS node_data_field_cs_id_field_cs_id_value,
            node.type AS node_type,
            node.vid AS node_vid,
            node_data_field_cs_id.field_cs_forward_primer_nid AS node_data_field_cs_id_field_cs_forward_primer_nid,
            node_data_field_cs_id.field_cs_reverse_primer_nid AS node_data_field_cs_id_field_cs_reverse_primer_nid,
            node_data_field_cs_id.field_cs_mid_value AS node_data_field_cs_id_field_cs_mid_value,
            node.uid AS node_uid,
            node_revisions.format AS node_revisions_format,
            node_data_field_cs_id.field_cs_wl_value AS node_data_field_cs_id_field_cs_wl_value
            FROM node node 
            LEFT JOIN nodehierarchy_menu_links nodehierarchy_menu_links ON node.nid = nodehierarchy_menu_links.nid
            LEFT JOIN menu_links nh_menu_links ON nodehierarchy_menu_links.mlid = nh_menu_links.mlid
            LEFT JOIN nodehierarchy_menu_links nh_parent ON nh_menu_links.plid = nh_parent.mlid
            LEFT JOIN content_type_pdb_client_sample node_data_field_cs_id ON node.vid = node_data_field_cs_id.vid
            LEFT JOIN node_revisions node_revisions ON node.vid = node_revisions.vid
            WHERE (node.status = 1) AND (nh_parent.nid = $nid )
            ORDER BY node_data_field_cs_id_field_cs_id_value ASC";
            
            
        $javascript = '<script language="JavaScript">
        function checkAll(form) {
            var elements = form.elements;
            for (var i = 0; i < elements.length; i++) {
                if (elements[i].type == "checkbox") {
                    elements[i].checked = true ;
                }
            }
        }        
        function uncheckAll(form) {
            var elements = form.elements;
            for (var i = 0; i < elements.length; i++) {
                if (elements[i].type == "checkbox") {
                    elements[i].checked = false ;
                }
            }
        }
        </script>
        ';
                
        $samples = array();
        $result = db_query($query);
        
        while ($row = db_fetch_object($result)) {
            $samples[$row->nid] = t($row->node_title . " (UID:" . $row->node_data_field_cs_id_field_cs_id_value. ")");
        }     

        $form['samples'] = array('#type' => 'checkboxes',
                                 '#options' => $samples,
                                 '#title' => t("Select samples to clone"),
                                 '#prefix' => t($javascript));
        $form['newtitle'] = array('#type' => 'textfield',
                                 '#title' => t('Cloned Job Title'));
        $form['checkall'] = array('#type' => 'button',
                                  '#attributes' => array('onclick' => t('checkAll(this.form); return false;')),
                                  '#value' => t('Select all'));
        
        $form['uncheckall'] = array('#type' => 'button',
                                    '#attributes' => array('onclick' => t('uncheckAll(this.form); return false;')),
                                    '#value' => t('Unselect all'));
        
        $form['cancel'] = array('#type' => 'submit', '#value' => t('Cancel'));
        $form['submit'] = array('#type' => 'submit', '#value' => t('Clone!'));
    } else {
        $view = views_get_view('job_sample_table');
        $view_html = $view->execute_display('default', array($nid));
      
        $form['htmldummy'] = array('#type' => 'hidden',
                                   '#value' => 'dummy',
                                   '#prefix' => t($view_html));

        $result = db_query("SELECT wn.sid FROM {workflow_node} wn WHERE wn.nid = '$nid'");
        while ($row = db_fetch_object($result)) {
        if ($row->sid == 2 and _pyro_db_is_admin()) // State for unlocked job 
            {
                $form['submit'] = array('#type' => 'submit',
                                        '#value' => t('Clone Job!'),
                                        '#prefix' => "<br><h1>CLONE JOB</h1>");
            }
        }
    }
    $form['nid'] = array(
        '#type' => 'hidden',
        '#value' => $nid,
    );
    return $form;
}

function _pyro_db_clone_job_submit($form, &$form_state) {
    if($form_state['storage']['values'] != NULL) {
        if ($form_state['values']['op'] == "Clone!") {
            $new_job_nid = _pyro_db_clone_job($form_state['clicked_button']['#post']['nid'],
                                              $form_state['clicked_button']['#post']['samples'],
                                              $form_state['clicked_button']['#post']['newtitle']);
            $form_state['storage']['values'] = NULL;
            #header("Location: /content/node/$new_job_nid");
        } else {
            $form_state['storage']['values'] = NULL;
        };
    } elseif ($form_state['values']['op'] == "Cancel") {
        $form_state['storage']['values'] = NULL;
    } else {
        $form_state['storage']['values'] = $form_state['values'];
    }
}

function _pyro_db_clone_job($job_nid, $sample_nids, $new_job_title) {

    $result = db_query("SELECT n.title, n.uid, u.name
                       FROM {node} n
                       LEFT JOIN {users} u
                       ON n.uid = u.uid
                       WHERE n.nid = '$job_nid'");
    $row = db_fetch_object($result);
    
    $job_owner = new stdClass();
    $job_owner->name = $row->name;
    $job_owner->uid = $row->uid;
    
    // Create a new sample node
    $std_node = new stdClass();
    $std_node->type = 'pdb_client_job';
    $std_node->title =  $new_job_title;
    $std_node->created = time();
    $std_node->changed = $std_node->created;
    $std_node->format = 3;              // PHP
    $std_node->name = $job_owner->name;      // <----- use $user->name otherwise you get 'Anonymous'
    $std_node->uid = $job_owner->uid;
    node_save($std_node); // <--- Save the node now
    #auto_nodetitle_operations_update(array($std_node->nid));            // <---- fix the title
    
    $nodehierarchy_menu_links = _nodehierarchy_default_menu_link(0, _nodehierarchy_get_node_mlid($job_nid,true));
    $nodehierarchy_menu_links['hidden'] = TRUE;
    $nodehierarchy_menu_links['enabled'] = FALSE; //  <------ Make sure the menu link is enabled
    $std_node->nodehierarchy_menu_links[] = $nodehierarchy_menu_links;
    _nodehierarchy_save_node($std_node); // <---- Let nodeheirarchy do it's thing
    
    $new_job_nid = $std_node->nid;
    
    # Creation of a job will have created a default sample, this needs to be identified (so it can be deleted later).
    $default_samples = _pyro_db_job_get_all_samples($new_job_nid);
    
    foreach ($sample_nids as $sample_nid) {
            $new_sample_nid = _pyro_db_make_next_sample($job_owner, $new_job_nid);
            _pyro_db_clone_sample_guts($sample_nid, $new_sample_nid);
            
            # Fix the node title
            $original_node = node_load($sample_nid);
            $query = "UPDATE {node} n SET n.title='".$original_node->title."' WHERE nid='".$new_sample_nid."'";
            db_query($query);
            
            # Fix the node revision title
            $query = "UPDATE {node_revisions} nr SET nr.title='".$original_node->title."' WHERE nid='".$new_sample_nid."'";
            db_query($query);
           
            # Fix the menu links...
            # Firstly find the menu link ID for the new sample node
            $query = "SELECT ml.mlid
                      FROM {menu_links} ml
                      LEFT JOIN {nodehierarchy_menu_links} nhml
                      ON nhml.mlid = ml.mlid
                      WHERE nhml.nid = '$new_sample_nid'";
            $result = db_query($query);
            $row = db_fetch_object($result);
            
            # Then, update the menu link table.
            $query = "UPDATE {menu_links} ml SET ml.link_title='".$original_node->title."' WHERE mlid='".$row->mlid."'";
            db_query($query);
            
    }
    
    foreach ($default_samples as $sample_nid) {
        node_delete($sample_nid);
    }
    
    #Fix the job info.
    $query = "SELECT cj.field_cj_description_value,
                     cj.field_cj_chem_value,
                     cj.field_cj_affiliation_nid,
                     cj.field_cj_full_name_value,
                     cj.field_cj_lead_invest_uid
              FROM {content_type_pdb_client_job} cj
              WHERE cj.nid = '$job_nid'";
    $result = db_query($query);
    $row = db_fetch_object($result);
    

    $query = "UPDATE {content_type_pdb_client_job} SET " .
             "field_cj_description_value = '" . $row->field_cj_description_value . "'," .
             "field_cj_chem_value = '" . $row->field_cj_chem_value . "'," .
             "field_cj_samp_num_value = '" . count($sample_nids) . "'," .
             "field_cj_affiliation_nid = '" . $row->field_cj_affiliation_nid . "'," .
             "field_cj_full_name_value = '" . $row->field_cj_full_name_value . "'," .
             "field_cj_lead_invest_uid = '" . $row->field_cj_lead_invest_uid . "'," .
             "field_cj_gasket_nid='NULL'," .
             "field_cj_samples_arrived_value='Awaiting arrival' " .
             "WHERE nid = '$new_job_nid'";
    
    db_query($query);

    return $std_node->nid;       
} 


function _pyro_db_clone_job_validate($form, &$form_state) {    
    if(isset($form_state['storage']['values']) and ($form_state['values']['op'] == "Clone!")) {
        if (count($form_state['clicked_button']['#post']['samples']) == 0) {
            form_set_error('', t('You must select at least one sample to clone.'));
        }
        if (! $form_state['clicked_button']['#post']['newtitle']) {
            form_set_error('', t('You must specify a title for the cloned job.'));
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Misc
//////////////////////////////////////////////////////////////////////////////////////////////////

function _pyro_db_job_delete($nid) {
    //-----
    // called just before a job is deleted
    //
    // remove this guy from the tables table
    db_query("DELETE FROM {pdb_metadata_tables}  where nid='".$nid."'");
}
