<?php
// $Id$

/*#############################################################################
#
#    pyro_db.clients.inc
#    
#    Code which implements functionality for client based objects.
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

function get_max_client_group_billing_id_cck_form() {
    return array(
      0 => array('value' => get_max_client_group_billing_id() + 1)
    );
};


function get_max_client_group_billing_id() {
    $max = 1;
    $result = db_query("SELECT field_cg_bill_id_value FROM {content_type_pdb_client_group}");
    while ($row = db_fetch_object($result)) {
        if (! is_numeric($row->field_cg_bill_id_value)) {
            return -1;
        }
        if ($max < $row->field_cg_bill_id_value) {
            $max = $row->field_cg_bill_id_value;
        }
    }
    return $max;
}