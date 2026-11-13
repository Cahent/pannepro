<?php
namespace App\Models;
use Illuminate\Database\Eloquent\Model;
class StatusHistory extends Model {
    public $timestamps=false;
    protected $fillable=['provider_id','device_id','status','capacity','created_at'];
}
