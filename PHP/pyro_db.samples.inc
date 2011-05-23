<?php
// $Id$
/*#############################################################################
#
#    pyro_db_samples.inc
#    
#    Code which implements funcitonality for pyrotag samples
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

putenv("TZ=Australia/Brisbane");

//////////////////////////////////////////////////////////////////////////////////////////////////
// Includes for each table
//////////////////////////////////////////////////////////////////////////////////////////////////
include_once('./'. drupal_get_path('module', 'pyro_db') .'/autocode/pdb_metadata_air.inc');
include_once('./'. drupal_get_path('module', 'pyro_db') .'/autocode/pdb_metadata_host_associated.inc');
include_once('./'. drupal_get_path('module', 'pyro_db') .'/autocode/pdb_metadata_human_associated.inc');
include_once('./'. drupal_get_path('module', 'pyro_db') .'/autocode/pdb_metadata_human_gut.inc');
include_once('./'. drupal_get_path('module', 'pyro_db') .'/autocode/pdb_metadata_human_oral.inc');
include_once('./'. drupal_get_path('module', 'pyro_db') .'/autocode/pdb_metadata_human_skin.inc');
include_once('./'. drupal_get_path('module', 'pyro_db') .'/autocode/pdb_metadata_human_vaginal.inc');
include_once('./'. drupal_get_path('module', 'pyro_db') .'/autocode/pdb_metadata_mat_biofilm.inc');
include_once('./'. drupal_get_path('module', 'pyro_db') .'/autocode/pdb_metadata_miscellaneous.inc');
include_once('./'. drupal_get_path('module', 'pyro_db') .'/autocode/pdb_metadata_plant_associated.inc');
include_once('./'. drupal_get_path('module', 'pyro_db') .'/autocode/pdb_metadata_sediment.inc');
include_once('./'. drupal_get_path('module', 'pyro_db') .'/autocode/pdb_metadata_soil.inc');
include_once('./'. drupal_get_path('module', 'pyro_db') .'/autocode/pdb_metadata_wastewater_sludge.inc');
include_once('./'. drupal_get_path('module', 'pyro_db') .'/autocode/pdb_metadata_water.inc');


//////////////////////////////////////////////////////////////////////////////////////////////////
// Body
//////////////////////////////////////////////////////////////////////////////////////////////////

function _pyro_db_sample_body($isSave, $nid) {
    if($isSave)
    {
        // called when the node is being saved or updated...
        
        // overwrite the body of the node so that we own it!
        $prog_body = "<"."?php _pyro_db_sample_body(FALSE, $nid); ?".">";
        db_query("UPDATE {node_revisions} nr SET nr.body='".$prog_body."', nr.teaser='', nr.format='3' WHERE nid='".$nid."'");
    }
    else
    {
        # print the miscellaneous fields
        _pyro_db_fields_as_table($nid);

        # print any custom metadata
        $metadata_exists = false;
        echo "<br><h1>METADATA</h1><hr>";
        $result = db_query("SELECT * FROM {pdb_metadata_tables}  where nid='".$nid."'");
        while ($row = db_fetch_object($result)) {
            foreach ($row as $key => $value)
            {
                if(1 == $value)
                {
                    # make the funciton name
                    $func = "_pyro_db_".$key."_view";
                    # call the function
                    $func($nid);
                    $metadata_exists = true;
                }
            }
            break;
        }

        if(!$metadata_exists) { drupal_set_message("WARNING: No metadata fields selected for this sample", 'warning', true); }

        // called when a node is being viewed
        $ret_str = '';

        // if the node is unlocked then we can clone!
        $result = db_query("SELECT wn.sid FROM {workflow_node} wn WHERE wn.nid = '$nid'");
        while ($row = db_fetch_object($result)) {
            if($row->sid == 5)
            {
                echo "<br><h1>CLONE SAMPLE</h1><hr>";
                print drupal_get_form('_pyro_db_cloneform', $nid);      
            }
        }
        
        // link back to the parent job
        $job_link = '';
        $job_title = '';
        $result = db_query("SELECT nhml.mlid FROM {nodehierarchy_menu_links} nhml WHERE nhml.nid = '$nid'");
        while ($row = db_fetch_object($result)) {
            $result2 = db_query("SELECT ml.plid FROM {menu_links} ml WHERE ml.mlid = '".$row->mlid."'");
            while ($row2 = db_fetch_object($result2)) {
                $result3 = db_query("SELECT ml.link_title, ml.link_path FROM {menu_links} ml WHERE ml.mlid = '".$row2->plid."'");
                while ($row3 = db_fetch_object($result3)) {
                    $job_title = $row3->link_title;
                    $result4 = db_query("SELECT ua.dst FROM {url_alias} ua WHERE ua.src = '".$row3->link_path."'");
                    while($row4 = db_fetch_object($result4)) {
                        $job_link = $row4->dst;
                        break;
                    }
                    break;
                }
                break;
            }
            break;
        }
        $ret_str .= '<br><br><a href="?q='.$job_link.'">Back to Job: '.$job_title.'</a><br>';
        
        echo $ret_str;
        
    }
}

function _pyro_db_fields_as_table($nid) {
    //-----
    // View the available fields in a themed table
    //
    $fancy_names = _pyro_db_get_fields();
    $chem = _pyro_db_get_parent_job_chem($nid);
    $nn_fields = _pyro_db_get_non_null_cck_fields($nid);
    $header = array('Field', 'Value');
    $rows = array();
    foreach($nn_fields as $key => $value) {
        if($key == 'field_cs_coll_date_value') { $value = format_date(strtotime($value), 'large'); }
        elseif($key == 'field_cs_forward_primer_nid') { $value = _pyro_db_sample_get_primer_string($chem, '454', 'forward', $nn_fields['field_cs_mid_value'], $nid, $value, 'sample page'); }
        elseif($key == 'field_cs_reverse_primer_nid') { $value = _pyro_db_sample_get_primer_string($chem, '454', 'reverse', $nn_fields['field_cs_mid_value'], $nid, $value, 'sample page'); }
        elseif($key == 'field_cs_mid_value') { if('' == $value or $value == NULL) { $value = " MID UNSET"; } }
        $rows[] = array( array('data' => '<B>'.$fancy_names[$key]['name'].'</B>', 'width' => '35%') , $value);
    }
    $view = views_get_view('gmap_loc_view');
    $rows[] = array( array('data' => '<B>Location</B>', 'width' => '35%') , $view->execute_display('default', array($nid)));
    
    echo "<br><h1>MAIN INFORMATION</h1><hr>";
    echo theme_table($header, $rows);
    return;
}

function _pyro_db_sample_get_primer_string($chem_type, $machine_type, $pri_dir, $mid, $nid, $pri_nid, $view_type) {
    //-----
    // format the display string of the pyrotag primer
    //
    $ret_str = "ERROR: NOT FOUND";
    if('' == $mid or $mid == NULL)
    {
        $mid = "-MIDXX-";
    }
    $result = db_query("SELECT ctpp.field_pp_pri_name_value, ctpp.field_pp_adaptor_value, ctpp.field_pp_key_value, ctpp.field_pp_template_value FROM {content_type_pyrotag_primer} ctpp WHERE nid ='$pri_nid'");
    if($view_type == 'sample page')
    {
        while ($row = db_fetch_object($result)) 
        {
            if($machine_type == '454')
            {
                if($chem_type == 'L')
                {
                    if($pri_dir == 'reverse')
                    {
                        $ret_str = $row->field_pp_pri_name_value . " -- " . $row->field_pp_adaptor_value.ctpp.$row->field_pp_key_value.$mid.$row->field_pp_template_value;
                    }
                    else
                    {
                        $ret_str = $row->field_pp_pri_name_value . " -- " . $row->field_pp_adaptor_value.ctpp.$row->field_pp_key_value.$row->field_pp_template_value;
                    }
                }
                else // $chem_type == 'A'
                {
                    if($pri_dir == 'reverse')
                    {
                        $ret_str = $row->field_pp_pri_name_value . " -- " . $row->field_pp_adaptor_value.ctpp.$row->field_pp_key_value.$row->field_pp_template_value;
                    }
                    else
                    {
                        $ret_str = $row->field_pp_pri_name_value . " -- " . $row->field_pp_adaptor_value.ctpp.$row->field_pp_key_value.$row->field_pp_template_value;
                    }
                }
            } 
            else // $machine_type == 'ION'
            {
            }
        }
    }
    return $ret_str;
}

function _pyro_db_get_non_null_cck_fields($nid) {
    //-----
    // get all the active fields and retuen them as an array
    //
    $active_fields = _pyro_db_get_fields();
    $ret_array = array();
    
    $result = db_query("SELECT * from {content_type_pdb_client_sample} WHERE nid = '$nid'");
    if ($tmp_array = db_fetch_object($result)) {
        foreach($tmp_array as $key => $value) {
            if($active_fields[$key]['active'])
            {
                if(NULL != $value) {
                    $ret_array[$key] = $value;
                }
            }
        }
    }
    
    return $ret_array;
}

function _pyro_db_set_non_null_cck_fields($update_values, $nid) {
    //-----
    // get all the active fields and retuen them as an array
    //
    $active_fields = _pyro_db_get_fields();
    $ins_query = "UPDATE {content_type_pdb_client_sample} SET ";
    $db_vars = array();
    $first = true;
    
    foreach($update_values as $key => $value) 
    {
        if($active_fields[$key]['active'])
        {
            if(NULL != $value) {
                // add to the query
                if(!$first) { $ins_query .= ", "; }
                $first = false;
                $ins_query .= $key."='".$active_fields[$key]['type']."'";
                $db_vars[] = $update_values[$key];
            }
        }
    }
    $ins_query .= " where nid='$nid'";
    db_query($ins_query, $db_vars);
    return $ins_query;
}

function _pyro_db_get_fields() {
    //-----
    // get an array containing all the field information
    // YOU NEED TO UPFDATE THIS WHENEVER YOU ADD A NEW CCK TYPE TO THE
    // SAMPLES PAGE
    //
    return array(
        'field_cs_description_value' => array('name' => 'Description', 'active' => true, 'type' => '%s',),
        'field_cs_coll_date_value' => array('name' => 'Collection date', 'active' => true, 'type' => '%s',),
        'field_cs_forward_primer_nid' => array('name' => 'Forward primer', 'active' => true, 'type' => '%d',),
        'field_cs_reverse_primer_nid' => array('name' => 'Reverse primer', 'active' => true, 'type' => '%d',),
        'field_cs_mid_value' => array('name' => 'MID', 'active' => true, 'type' => '%s',),
        'field_cs_env_feature_value' => array('name' => 'Environmental Feature', 'active' => true, 'type' => '%s',),
        'field_cs_env_matter_value' => array('name' => 'Environmental Matter', 'active' => true, 'type' => '%s',),
        'field_cs_sub_insdc_value' => array('name' => 'Submitted to insdc', 'active' => true, 'type' => '%s',),
        'field_cs_investigation_type_value' => array('name' => 'Investigation Type', 'active' => true, 'type' => '%s',),
        'field_cs_experimental_factor_value' => array('name' => 'Experimental Factor', 'active' => true, 'type' => '%s',),
        'field_cs_biome_value' => array('name' => 'Environment (biome)', 'active' => true, 'type' => '%s',),
        'field_cs_subspecf_gen_lin_value' => array('name' => 'Subspecific Genetic Lineage', 'active' => true, 'type' => '%s',),
        'field_cs_extrachrom_elements_value' => array('name' => 'Extrachromosomal Elements', 'active' => true, 'type' => '%s',),
        'field_cs_source_mat_id_value' => array('name' => 'Source Material Identifiers', 'active' => true, 'type' => '%s',),
        'field_cs_biotic_relationship_value' => array('name' => 'Observed Biotic Relationship', 'active' => true, 'type' => '%s',),
        'field_cs_trophic_level_value' => array('name' => 'Trophic Level', 'active' => true, 'type' => '%s',),
        'field_cs_rel_to_oxygen_value' => array('name' => 'Relationship to Oxygen', 'active' => true, 'type' => '%s',),
        'field_cs_isol_growth_condt_value' => array('name' => 'Isolation and Growth Condition', 'active' => true, 'type' => '%s',),
        'field_cs_samp_collect_device_value' => array('name' => 'Sample Collection Device or Method', 'active' => true, 'type' => '%s',),
        'field_cs_samp_mat_proces_value' => array('name' => 'Sample Material Processing', 'active' => true, 'type' => '%s',),
        'field_cs_samp_size_value' => array('name' => 'Amount or Size of Sample Collected', 'active' => true, 'type' => '%s',),
        'field_cs_nucl_acid_ext_value' => array('name' => 'Nucleic Acid Extraction', 'active' => true, 'type' => '%s',),
        'field_cs_nucl_acid_amp_value'  => array('name' => 'Nucleic Acid Amplification', 'active' => true, 'type' => '%s',),
        'field_cs_id_value' => array('name' => 'Unique ID', 'active' => true, 'type' => '%s',),
        
    );
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Edit form
//////////////////////////////////////////////////////////////////////////////////////////////////

function _pyro_db_sample_is_human_meta($key) {
    //-----
    // check to see if the key contains the substring "human"
    //
    $pos = strpos($key,"human");

    if($pos === false) 
    {
        return false;
    }
    else 
    {
        return true;
    }
}

function _pyro_db_sample_form_alter(&$form, $form_state, $form_id) {
    //
    // Add metadata fileds to the sample edit form
    // Stop editing of primers when state is locked
    $nid = $form['nid']['#value'];

    // restrict primer changes when sample is locked
    if(_pyro_db_sample_is_locked($nid))
    {
        $form['field_cs_forward_primer']['#access'] = false;
        $form['field_cs_reverse_primer']['#access'] = false;
    }
    
    // restrict MID changes when Gasket is locked.
    if(_pyro_db_sample_gasket_is_locked($nid))
    {
        $form['field_cs_mid']['#access'] = false;
    }
    
    // add metadata
    // we need to make sure that we get this human business right
    // push all the keys to an aray and checkkk it out!
    
    $key_array = array();
    $hu_array = array();
    $do_human = false;
    $metadata_exists = false;
    
    $result = db_query("SELECT * FROM {pdb_metadata_tables}  where nid='".$nid."'");
    while ($row = db_fetch_object($result)) {
        foreach ($row as $key => $value)
        {
            if(1 == $value)
            {
                if(_pyro_db_sample_is_human_meta($key)) { $do_human = true; }
                $key_array[] = $key;
                $metadata_exists = true;
            }
        }
        break;
    }
    
    // now we make the metadata inputs!
    // do human if we need to!
    if($do_human)
    {
        _pyro_db_pdb_metadata_human_associated_form_alter($form, $form_state, $hu_array);
    }
    
    foreach ($key_array as $ordererer => $db_table)
    {
        if('pdb_metadata_human_associated' != $db_table)
        {
            # make the function name
            $func = "_pyro_db_".$db_table."_form_alter";
            
            # call the function (pass different rep arrays for human and other)
            if(_pyro_db_sample_is_human_meta($key)) { $func($form, $form_state, $hu_array); }
            else { $tmp_A = array(); $func($form, $form_state, $tmp_A); }
        }
    }
    
    $form['#validate'][] = '_pyro_db_sample_validate';
    $form['#submit'][] = '_pyro_db_sample_submit';
    
    if(!$metadata_exists) { drupal_set_message("WARNING: No metadata fields selected for this sample", 'warning', true); }
    
    // if the gasket is locked, do not allow deleting
    if(_pyro_db_sample_gasket_is_locked($nid))
    {
        unset($form['buttons']['delete']);
    }    
}

function _pyro_db_sample_validate($form, &$form_state) {
    //----
    // Validate use rinput on the sample edit page
    //
    $nid = $form['nid']['#value'];
    $result = db_query("SELECT * FROM {pdb_metadata_tables}  where nid='".$nid."'");
    while ($row = db_fetch_object($result)) {
        foreach ($row as $key => $value)
        {
            if(1 == $value)
            {
                # make the function name
                $func = "_pyro_db_".$key."_form_validate";
                # call the function
                $func($form, $form_state);
            }
        }
        break;
    }
}

function _pyro_db_sample_submit($form, &$form_state) {
    //-----
    // Update the sample with any changes to the metadata
    //     
    $nid = $form['nid']['#value'];
    
    // MID
    $current_mid = _pyro_db_sample_get_current_mid($nid);
    $form_mid = $form_state['values']['field_cs_mid'][0]['value'];
    if(!($form_mid === $current_mid))
    {
        _pyro_db_sample_update_mid($form_mid, $nid);
    }
    
    // metadata
    $result = db_query("SELECT * FROM {pdb_metadata_tables}  where nid='".$nid."'");
    while ($row = db_fetch_object($result)) {
        foreach ($row as $key => $value)
        {
            if(1 == $value)
            {
                # make the function name
                $func = "_pyro_db_".$key."_form_submit";
                # call the function
                $func($form, $form_state);
            }
        }
        break;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// job values
//////////////////////////////////////////////////////////////////////////////////////////////////

function _pyro_db_get_parent_job_nid($nid) {

    // get the NID of the parent job
    // default value
    $p_nid = 0;
    // get the menu link for this node
    $result = db_query("SELECT nhml.mlid FROM {nodehierarchy_menu_links} nhml WHERE nhml.nid = '$nid'");
    while ($row = db_fetch_object($result)) {
        // get the parents menu link id
        $result2 = db_query("SELECT ml.plid FROM {menu_links} ml WHERE ml.mlid = '".$row->mlid."'");
        while ($row2 = db_fetch_object($result2)) {
            // get the parents node id
            $result3 = db_query("SELECT nhml.nid FROM {nodehierarchy_menu_links} nhml WHERE nhml.mlid = '".$row2->plid."'");
            while ($row3 = db_fetch_object($result3)) {
                return $row3->nid;
            }
            break;
        }
        break;
    }
    // return default
    return $p_nid;
}

function _pyro_db_get_parent_job_chem($nid) {
    // get the chemistry type of the parent job
    // default value
    $chem = 'L';
    
    // get the parents node id
    $job_nid = _pyro_db_get_parent_job_nid($nid);
    
    // get the chemistry of the parent
    $result = db_query("SELECT ccj.field_cj_chem_value FROM {content_type_pdb_client_job} ccj WHERE ccj.nid = '$job_nid'");
    while ($row = db_fetch_object($result)) {
        $chem = $row->field_cj_chem_value;
        break;
    }
    // return default
    return $chem;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Sample Cloning
//////////////////////////////////////////////////////////////////////////////////////////////////

function _pyro_db_cloneform($form_state, $nid) {
  // Access log settings:
  $form['#submit'][] = '_pyro_db_clone_sample_submit';
  $form['#validate'][] = '_pyro_db_clone_sample_validate';
  
  $form['nid'] = array(
  '#type' => 'hidden',
  '#value' => $nid,
  );
  
  $clone_opts = drupal_map_assoc(range(1,100));
  $form['num_clones'] = array(
    '#type' => 'select',
    '#options' => $clone_opts,
    '#title' => t('Clone this Sample'),
    '#size' => 1,
    '#description' => t('Enter the number of clones you wish to make'),
  );
  
  $form['submit'] = array('#type' => 'submit', '#value' => t('Clone!'));
  
  return $form;
}

function _pyro_db_clone_sample_submit($form, &$form_state) {
    $num_clones = $form_state['values']['num_clones'];
    $nid = $form_state['values']['nid'];
    // get the parent nid
    $parent_nid = -1;
    $result = db_query("SELECT nhml.mlid FROM {nodehierarchy_menu_links} nhml WHERE nhml.nid = '$nid'");
    while ($row = db_fetch_object($result)) {
        $result2 = db_query("SELECT ml.plid FROM {menu_links} ml WHERE ml.mlid = '".$row->mlid."'");
        while ($row2 = db_fetch_object($result2)) {
            $result3 = db_query("SELECT nhml.nid FROM {nodehierarchy_menu_links} nhml WHERE nhml.mlid = '".$row2->plid."'");
            while ($row3 = db_fetch_object($result3)) {
                $parent_nid = $row3->nid;
                break;
            }
            break;
        }
        break;
    }
    if(-1 != $parent_nid)
    {
        global $user;
        $count = 0;
        while($count < $num_clones)
        {
            $new_nid = _pyro_db_make_next_sample($user, $parent_nid);
            _pyro_db_clone_sample_guts($nid, $new_nid);
            $count++;
        }
        drupal_set_message('Made '.$num_clones.' copie(s) of this sample ', 'status', TRUE);
    }
    else
    {
        form_set_error("An internal error occurred");
    }
}

function _pyro_db_clone_sample_validate($form, &$form_state) {
    // nothing much to do here...
}

function _pyro_db_clone_sample_guts($src_nid, $dst_nid) {
    // get all the content in the src node and make this the content of the dst node
    // this needs to be updated whenever a new field is added
    $clone_values = _pyro_db_get_non_null_cck_fields($src_nid);
    $reet = _pyro_db_set_non_null_cck_fields($clone_values, $dst_nid);
    
    // clone all the metadata
    $result = db_query("SELECT * FROM {pdb_metadata_tables}  where nid='".$src_nid."'");
    while ($row = db_fetch_object($result)) {
        foreach ($row as $key => $value)
        {
            if(1 == $value)
            {
                
                # make the funciton name
                $func = "_pyro_db_".$key."_clone";
                # call the function
                $func($src_nid, $dst_nid);
            }
        }
        break;
    }
    
    // finally, clone the location
    // the default location will be set, we just need to update the latitude, longditude and country info...
    $loc_array = _pyro_db_get_location_by_sample_nid($src_nid);
    // get the lid
    $lid = 0;
    $result = db_query("SELECT cs.field_cs_location_lid FROM {content_type_pdb_client_sample} cs WHERE nid='$dst_nid'");
    while ($row = db_fetch_object($result)) 
    {
        $lid = $row->field_cs_location_lid;
        break;
    }
    $ins_query = "UPDATE {location} set country = '%s', latitude = '%f', longitude = '%f' WHERE lid = '$lid'";
    db_query($ins_query, array($loc_array['country'], $loc_array['latitude'], $loc_array['longitude']));

}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Locations
//////////////////////////////////////////////////////////////////////////////////////////////////

function _pyro_db_set_new_location(&$locArray) {
    //-----
    // makes a new location
    // Can be used to make a default value or to clone!
    //
    // $genStr should look like: "cck:field_cs_location:182" where 182 is the nid given in the function call
    //
    // ARRAY is: ($nid, $vid, $uid, $country, $lat, $long, $genStr)
    // first make a new location in the db
    $ins_query = "INSERT INTO {location} (country, latitude, longitude, source, is_primary) VALUES ('%s', '%f', '%f', '%d', '%d')";
    db_query($ins_query, array($locArray['country'], $locArray['latitude'], $locArray['longitude'], 1, 0));
    
    $lid =  db_last_insert_id('location', 'lid');

    // now update the location instance table
    $ins_query = "INSERT INTO {location_instance} (nid, vid, uid, genid, lid) VALUES ('%d', '%d', '%d', '%s', '%d')";
    db_query($ins_query, array($locArray['nid'], $locArray['vid'], $locArray['uid'], $locArray['genStr'], $lid));
    
    return $lid;
}

function _pyro_db_get_client_group_nid($uid) {
    //-----
    // return the ID of the client group for a given uid
    //
    $profile_node = content_profile_load('profile', $uid);
    $nid = $profile_node->nid;
    $result = db_query("SELECT ctp.field_user_cg_nid FROM {content_type_profile} ctp WHERE nid='$nid'");
    while ($row = db_fetch_object($result)) 
    {
        return $row->field_user_cg_nid;
    }
    
    return 0;
}

function _pyro_db_get_location_by_cgid($cg_nid) {
    //-----
    // get an array of location information via a client group id
    //
    $result = db_query("SELECT ctpcg.field_cg_address_lid FROM {content_type_pdb_client_group} ctpcg WHERE nid='$cg_nid'");
    while ($row = db_fetch_object($result)) 
    {
        return _pyro_db_get_location_by_lid($row->field_cg_address_lid);
    }
    return array();
}

function _pyro_db_get_location_by_lid($lid) {
    //-----
    // get an array of location information via a location id
    //
    $ret_array = array();
    $result = db_query("SELECT lt.latitude, lt.longitude, lt.country FROM {location} lt WHERE lid='$lid'");
    while ($row = db_fetch_object($result)) 
    {
        $ret_array['latitude'] = $row->latitude;
        $ret_array['longitude'] = $row->longitude;
        $ret_array['country'] = $row->country;
        break;
    }
    return $ret_array;
}

function _pyro_db_get_location_by_uid($uid) {
    //-----
    // get an array of location information via a sample nid
    // Only call this for samples
    //
    // get the vid of this node
    // get the locaton information
    $loc_array = _pyro_db_get_location_by_cgid(_pyro_db_get_client_group_nid($uid));
    @$loc_array['uid'] = $uid;
    return $loc_array;
}

function _pyro_db_get_location_by_sample_nid($nid) {
    //-----
    // get an array of location information via a sample nid
    // Only call this for samples
    //
    // get the vid of this node
    // get the locaton information
    
    $result = db_query("SELECT cs.field_cs_location_lid FROM {content_type_pdb_client_sample} cs WHERE nid='$nid'");
    while ($row = db_fetch_object($result)) 
    {
        return _pyro_db_get_location_by_lid($row->field_cs_location_lid);
    }
    return array();
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// MIDs
//////////////////////////////////////////////////////////////////////////////////////////////////

function _pyro_db_sample_set_mid($gs_nid, $mid, $samp_nid) {
    db_query("UPDATE {content_type_pdb_client_sample} ctpcs SET ctpcs.field_cs_mid_value = '$mid' WHERE nid = '$samp_nid'");
    db_query("INSERT INTO {pdb_gasket_mids} (gasket_id, mid, sample_id) VALUES ('$gs_nid', '$mid', '$samp_nid')");
    $tmp_sample_node = node_load($samp_nid);
    drupal_set_message("Sample: " . $tmp_sample_node->title . " assigned MID: '$mid'", 'status', true );
}

function _pyro_db_sample_update_mid($mid, $samp_nid) {
    db_query("UPDATE {content_type_pdb_client_sample} ctpcs SET ctpcs.field_cs_mid_value = '$mid' WHERE nid = '$samp_nid'");
    db_query("UPDATE {pdb_gasket_mids} pgm set pgm.mid = '$mid' where sample_id = '$samp_nid'");
    $tmp_sample_node = node_load($samp_nid);
    drupal_set_message("Sample: " . $tmp_sample_node->title . " assigned MID: '$mid'", 'status', true );
}

function _pyro_db_sample_unset_mid($samp_nid) {
    //-----
    // Unset the MID and fix the sample count for the gasket
    //
    $mid = '';
    $result = db_query("SELECT ctpcs.field_cs_mid_value FROM {content_type_pdb_client_sample} ctpcs WHERE nid = '$samp_nid'");
    while ($row = db_fetch_object($result)) {
        $mid = $row->field_cs_mid_value;
    }
    
    db_query("UPDATE {content_type_pdb_client_sample} ctpcs SET ctpcs.field_cs_mid_value = '' WHERE nid = '$samp_nid'");
    
    // we need to decrement the number used for this gasket.
    // get the gasket nid
    $result = db_query("SELECT pgm.gasket_id FROM {pdb_gasket_mids} pgm WHERE sample_id = '$samp_nid'"); 
    while ($row = db_fetch_object($result)) {
        $result2 = db_query("SELECT ctpg.field_gas_num_samps_value FROM {content_type_pdb_gasket} ctpg  WHERE nid = '$row->gasket_id'"); 
        while ($row2 = db_fetch_object($result2)) {
            $updated_num = $row2->field_gas_num_samps_value - 1;
            db_query("UPDATE {content_type_pdb_gasket} SET field_gas_num_samps_value = '$updated_num'  WHERE nid = '$row->gasket_id'");
            break;
        }
        break;
    }
    # now delete away...
    db_query("DELETE FROM {pdb_gasket_mids} WHERE sample_id = '$samp_nid'");
    $tmp_sample_node = node_load($samp_nid);
    drupal_set_message("MID '$mid' removed from Sample: " . $tmp_sample_node->title, 'status', true );
}

function _pyro_db_sample_get_current_mid($samp_nid) {
    //-----
    // wrapper to get the current mid for this sample
    // returns '' is no mid set.
    //
    $result = db_query("SELECT ctpcs.field_cs_mid_value FROM {content_type_pdb_client_sample} ctpcs where nid='$samp_nid'");
    while ($row = db_fetch_object($result)) 
    {
        if('' != $row->field_cs_mid_value)
        {
            return $row->field_cs_mid_value;
        }
    }
    return '';
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Misc
//////////////////////////////////////////////////////////////////////////////////////////////////

function _pyro_db_make_next_sample($user, $parent_nid) {
    
    $plid = _nodehierarchy_get_node_mlid($parent_nid,true); //    <-------- Important don't forget the 'true'.
    
    // Increment the number of samples counter
    $samp_num = _pyro_db_increment_job_sample_counter($parent_nid);
    
    // Get the title of the parent node
    $std_title = 'NOT_SET';
    $result = db_query("SELECT n.title FROM {node} n WHERE n.nid = '$parent_nid'");
    while ($row = db_fetch_object($result)) {
        $std_title = $row->title;
        break;
    }
        
    // Make a title for the new sample node
    $std_title .= "_Sample_$samp_num";
    
    // Create a new sample node
    $std_node = new stdClass();
    $std_node->type = 'pdb_client_sample';
    $std_node->title = $std_title;
    $std_node->created = time();
    $std_node->changed = $std_node->created;
    $std_node->format = 3;              // PHP
    $std_node->name = $user->name;      // <----- use $user->name otherwise you get 'Anonymous'
    $std_node->uid = $user->uid;
    node_save($std_node); // <--- Save the node now
    auto_nodetitle_operations_update(array($std_node->nid));            // <---- fix the title
    
    $nodehierarchy_menu_links = _nodehierarchy_default_menu_link($parent_nid, $plid);
    $nodehierarchy_menu_links['pnid']=$parent_nid;   // <----- Give the menu link the parent id
    $nodehierarchy_menu_links['hidden'] = FALSE;
    $nodehierarchy_menu_links['enabled'] = TRUE; //  <------ Make sure the menu link is enabled
    $std_node->nodehierarchy_menu_links[] = $nodehierarchy_menu_links;
    _nodehierarchy_save_node($std_node); // <---- Let nodeheirarchy do it's thing
    
    
    _pyro_db_sample_body($std_node->nid, true);
    return $std_node->nid;        
}

function _pyro_db_sample_delete($nid) {
    //-----
    // called just before a sample is deleted
    //
    // remove his MID
    _pyro_db_sample_unset_mid($nid);
    
    $result = db_query("SELECT * FROM {pdb_metadata_tables}  where nid='".$nid."'");
    while ($row = db_fetch_object($result)) {
        foreach ($row as $key => $value)
        {
            if(1 == $value)
            {
                # make the funciton name
                $func = "_pyro_db_".$key."_delete";
                # call the function
                $func($nid);
            }
        }
        break;
    }
    
    // remove this guy from the tables table
    db_query("DELETE FROM {pdb_metadata_tables}  where nid='".$nid."'");

}

function _pyro_db_sample_is_locked($nid) {
    //-----
    // Is this sample locked?
    // state 5 is open, state 6 is locked!
    //
    $result = db_query("SELECT wn.sid FROM {workflow_node} wn WHERE nid = '$nid'");
    while ($row = db_fetch_object($result)) {
        if($row->sid == 5) { return false; }
        elseif ($row->sid == 6) { return true; }
    }
    return false;
}

function _pyro_db_sample_gasket_is_locked($nid) {
    //-----
    // Is this samples' gasket locked?
    //
    $result = db_query("SELECT pgm.gasket_id FROM {pdb_gasket_mids} pgm WHERE sample_id = '$nid'"); 
    while ($row = db_fetch_object($result)) {
        $result2 = db_query("SELECT wn.sid FROM {workflow_node} wn WHERE nid = '$row->gasket_id'");
        while ($row2 = db_fetch_object($result2)) {
            if($row2->sid == 8) { return false; }
            elseif ($row2->sid == 9) { return true; }
        }
    }
    return false;
}
