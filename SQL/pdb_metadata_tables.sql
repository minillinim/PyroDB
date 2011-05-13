CREATE TABLE IF NOT EXISTS `pdb_metadata_tables` (
  `nid` int(10) unsigned NOT NULL,
  `pdb_metadata_air` tinyint(1) DEFAULT '0',
  `pdb_metadata_host_associated` tinyint(1) DEFAULT '0',
  `pdb_metadata_human_associated` tinyint(1) DEFAULT '0',
  `pdb_metadata_human_gut` tinyint(1) DEFAULT '0',
  `pdb_metadata_human_oral` tinyint(1) DEFAULT '0',
  `pdb_metadata_human_skin` tinyint(1) DEFAULT '0',
  `pdb_metadata_human_vaginal` tinyint(1) DEFAULT '0',
  `pdb_metadata_mat_biofilm` tinyint(1) DEFAULT '0',
  `pdb_metadata_miscellaneous` tinyint(1) DEFAULT '0',
  `pdb_metadata_plant_associated` tinyint(1) DEFAULT '0',
  `pdb_metadata_sediment` tinyint(1) DEFAULT '0',
  `pdb_metadata_soil` tinyint(1) DEFAULT '0',
  `pdb_metadata_wastewater_sludge` tinyint(1) DEFAULT '0',
  `pdb_metadata_water` tinyint(1) DEFAULT '0',
  PRIMARY KEY (`nid`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

