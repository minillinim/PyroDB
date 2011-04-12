SET SQL_MODE="NO_AUTO_VALUE_ON_ZERO";


/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8 */;

--
-- Table structure for table `pdb_metadata_tables`
--

CREATE TABLE IF NOT EXISTS `pdb_metadata_tables` (
  `nid` int(10) unsigned NOT NULL,
  `pdb_metadata_air` tinyint(1) DEFAULT '0',
  `pdb_metadata_host_associated_vertebrate` tinyint(1) DEFAULT '0',
  `pdb_metadata_human_associated` tinyint(1) DEFAULT '0',
  `pdb_metadata_host_sample` tinyint(1) DEFAULT '0',
  `pdb_metadata_sediment` tinyint(1) DEFAULT '0',
  `pdb_metadata_soil` tinyint(1) DEFAULT '0',
  `pdb_metadata_wastewater_sludge` tinyint(1) DEFAULT '0',
  `pdb_metadata_water` tinyint(1) DEFAULT '0',
  `pdb_metadata_common` tinyint(1) DEFAULT '0',
  PRIMARY KEY (`nid`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

