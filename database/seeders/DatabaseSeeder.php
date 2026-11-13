<?php
namespace Database\Seeders;
use App\Models\Region;
use Illuminate\Database\Seeder;
class DatabaseSeeder extends Seeder {
 public function run():void {
    foreach(['Nord'=>'nord','Süd'=>'sued','West'=>'west','Ost'=>'ost'] as $name=>$slug) Region::firstOrCreate(['slug'=>$slug],['name'=>$name]);
 }
}
