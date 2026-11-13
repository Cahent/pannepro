<?php
namespace App\Models;
use Illuminate\Database\Eloquent\Model;
class Region extends Model {
    protected $fillable=['name','slug'];
    public function providers() { return $this->belongsToMany(Provider::class, 'provider_region'); }
}
