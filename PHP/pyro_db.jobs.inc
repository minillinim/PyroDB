<?php
// $Id$
/**
*/
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
        // called when a node is being viewed
        $view = views_get_view('job_sample_table');
        print $view->execute_display('default', array($nid));
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
        '#weight' => 0, 
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
    
    if(NULL != $nid)
    {
        $form['lock_job'] = array(
            '#title' => 'Update this value ONLY when you have finsihed entering information for this job and ALL it\'s samples',
            '#type' => 'fieldset',
            '#collapsible' => TRUE,
            '#collapsed' => TRUE, 
            '#weight' => 100, 
            '#tree' => TRUE, 
        );
        
        $form['lock_job']['self_lock'] = array(
            '#title' => "I'm done!",
            '#description' => t('Check this box when you have finished adding information to this job and ALL of your samples. After the job has been locked you will no longer be able to edit any of the information, MIDs can be assigned to the samples and sequencing can commence. This can NOT be undone'),
            '#type' => 'checkbox',
            '#default_value' => 0,
        );
    }
    $form['#validate'][] = '_pyro_db_job_validate';
    $form['#submit'][] = '_pyro_db_job_submit';
    $form['buttons']['submit']['#submit'][] = '_pyro_db_job_redirect_handler';
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
        // not in creation. node_hook will hanle all the creation stuff
        $active_fields = _pyro_db_job_active_meta_fields($nid);
        $flipped_meta_fields = array_flip(_pyro_db_get_meta_fields());
        $sample_nids = _pyro_db_job_get_all_samples($nid);
        foreach ($form_state['values']['metadata_fields'] as $key => $value)
        {
            if(($value == 1) && ($active_fields[$key] == 0))
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
            elseif(($value == 0) && ($active_fields[$key] == 1))
            {
                // removed one
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
        workflow_execute_transition($job_node, 3);
        // do the samples
        $all_samples = _pyro_db_job_get_all_samples($job_node->nid);
        foreach ($all_samples as $this_sample)
        {
            $tmp_sample_node = node_load($this_sample);
            workflow_execute_transition($tmp_sample_node, 6);
        }
    }
    else
    {
        // do the job
        workflow_execute_transition($job_node, 2);
        // do the samples
        $all_samples = _pyro_db_job_get_all_samples($job_node->nid);
        foreach ($all_samples as $this_sample)
        {
            $tmp_sample_node = node_load($this_sample);
            workflow_execute_transition($tmp_sample_node, 5);
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Active metafields
//////////////////////////////////////////////////////////////////////////////////////////////////

function _pyro_db_get_meta_fields() {
    //-----
    // update this array to add / remove a metadata field
    //
    return array(
    'AIR' => 'pdb_metadata_air',
    'WATER' => 'pdb_metadata_water',
    'WASTE WATER SLUDGE' => 'pdb_metadata_wastewater_sludge',
    'HOST' => 'pdb_metadata_host',
    'HOST SAMPLE' => 'pdb_metadata_host_sample',
    'SOIL' => 'pdb_metadata_soil',
    'SEDIMENT' => 'pdb_metadata_sediment',
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